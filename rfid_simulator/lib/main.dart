import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:google_fonts/google_fonts.dart';
import 'package:nfc_manager/nfc_manager.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';

void main() => runApp(const RfidScannerApp());

class RfidScannerApp extends StatelessWidget {
  const RfidScannerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'RFID Scanner',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF0A0E21),
        colorScheme: ColorScheme.dark(
          primary: const Color(0xFF6C63FF),
          secondary: const Color(0xFF00D9F5),
          surface: const Color(0xFF1A1F38),
        ),
        textTheme: GoogleFonts.interTextTheme(ThemeData.dark().textTheme),
        inputDecorationTheme: InputDecorationTheme(
          filled: true,
          fillColor: const Color(0xFF1A1F38),
          border: OutlineInputBorder(
            borderRadius: BorderRadius.circular(14),
            borderSide: BorderSide(color: Colors.white12),
          ),
          enabledBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(14),
            borderSide: BorderSide(color: Colors.white12),
          ),
          focusedBorder: OutlineInputBorder(
            borderRadius: BorderRadius.circular(14),
            borderSide: BorderSide(color: const Color(0xFF6C63FF), width: 2),
          ),
          contentPadding: const EdgeInsets.symmetric(horizontal: 18, vertical: 16),
          labelStyle: const TextStyle(color: Colors.white54),
        ),
      ),
      home: const ScannerPage(),
    );
  }
}

class ScannerPage extends StatefulWidget {
  const ScannerPage({super.key});

  @override
  State<ScannerPage> createState() => _ScannerPageState();
}

class _ScannerPageState extends State<ScannerPage> with TickerProviderStateMixin {
  final _ipController = TextEditingController(text: '192.168.1.100');
  final _portController = TextEditingController(text: '1883');

  MqttServerClient? _mqttClient;
  bool _isConnected = false;
  bool _isConnecting = false;
  bool _isScanning = false;
  bool _nfcAvailable = false;
  String _lastTag = '';
  String _statusMessage = 'Not connected';
  final List<_ScanLog> _scanHistory = [];

  late AnimationController _pulseController;
  late Animation<double> _pulseAnimation;

  static const _topic = 'smartHome/devices/door/testCard';

  @override
  void initState() {
    super.initState();
    _pulseController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 2),
    )..repeat(reverse: true);
    _pulseAnimation = Tween<double>(begin: 0.8, end: 1.0).animate(
      CurvedAnimation(parent: _pulseController, curve: Curves.easeInOut),
    );
    _checkNfc();
  }

  Future<void> _checkNfc() async {
    final available = await NfcManager.instance.isAvailable();
    setState(() => _nfcAvailable = available);
  }

  @override
  void dispose() {
    _pulseController.dispose();
    _stopScanning();
    _mqttClient?.disconnect();
    _ipController.dispose();
    _portController.dispose();
    super.dispose();
  }

  // ── MQTT ──────────────────────────────────────────────

  Future<void> _connectMqtt() async {
    final ip = _ipController.text.trim();
    final port = int.tryParse(_portController.text.trim()) ?? 1883;
    if (ip.isEmpty) return;

    setState(() {
      _isConnecting = true;
      _statusMessage = 'Connecting to $ip:$port …';
    });

    _mqttClient = MqttServerClient(ip, 'rfid_scanner_${DateTime.now().millisecondsSinceEpoch}')
      ..port = port
      ..keepAlivePeriod = 30
      ..autoReconnect = true
      ..logging(on: false)
      ..onDisconnected = () {
        if (mounted) setState(() { _isConnected = false; _statusMessage = 'Disconnected'; });
      }
      ..onConnected = () {
        if (mounted) setState(() { _isConnected = true; _statusMessage = 'Connected to $ip:$port'; });
      };

    _mqttClient!.connectionMessage = MqttConnectMessage()
        .withClientIdentifier('rfid_scanner_${DateTime.now().millisecondsSinceEpoch}')
        .startClean();

    try {
      await _mqttClient!.connect();
      setState(() {
        _isConnected = true;
        _isConnecting = false;
        _statusMessage = 'Connected to $ip:$port';
      });
    } catch (e) {
      setState(() {
        _isConnecting = false;
        _isConnected = false;
        _statusMessage = 'Connection failed: ${e.toString().substring(0, (e.toString().length).clamp(0, 60))}';
      });
      _mqttClient?.disconnect();
    }
  }

  void _disconnectMqtt() {
    _mqttClient?.disconnect();
    setState(() {
      _isConnected = false;
      _statusMessage = 'Disconnected';
    });
  }

  void _publishTag(String tag) {
    if (_mqttClient == null || !_isConnected) return;
    final payload = jsonEncode({'cardTag': tag});
    final builder = MqttClientPayloadBuilder()..addString(payload);
    _mqttClient!.publishMessage(_topic, MqttQos.atLeastOnce, builder.payload!);
    setState(() {
      _scanHistory.insert(0, _ScanLog(tag: tag, time: DateTime.now()));
      if (_scanHistory.length > 20) _scanHistory.removeLast();
    });
  }

  // ── NFC ───────────────────────────────────────────────

  void _startScanning() {
    if (!_nfcAvailable) return;
    setState(() => _isScanning = true);
    NfcManager.instance.startSession(onDiscovered: (NfcTag tag) async {
      String tagId = '';
      final nfcA = tag.data['nfca'];
      final nfcB = tag.data['nfcb'];
      final iso15693 = tag.data['iso15693'];
      final iso7816 = tag.data['iso7816'];
      final ndef = tag.data['ndef'];

      // Try to get the tag identifier from various tech types
      if (nfcA != null && nfcA['identifier'] != null) {
        tagId = _bytesToHex(List<int>.from(nfcA['identifier']));
      } else if (nfcB != null && nfcB['identifier'] != null) {
        tagId = _bytesToHex(List<int>.from(nfcB['identifier']));
      } else if (iso15693 != null && iso15693['identifier'] != null) {
        tagId = _bytesToHex(List<int>.from(iso15693['identifier']));
      } else if (iso7816 != null && iso7816['identifier'] != null) {
        tagId = _bytesToHex(List<int>.from(iso7816['identifier']));
      } else if (ndef != null && ndef['identifier'] != null) {
        tagId = _bytesToHex(List<int>.from(ndef['identifier']));
      } else {
        // Fallback: check all data keys for identifier
        for (final key in tag.data.keys) {
          final val = tag.data[key];
          if (val is Map && val['identifier'] != null) {
            tagId = _bytesToHex(List<int>.from(val['identifier']));
            break;
          }
        }
      }

      if (tagId.isEmpty) tagId = 'UNKNOWN';

      setState(() => _lastTag = tagId);
      _publishTag(tagId);
    });
  }

  void _stopScanning() {
    if (!_nfcAvailable) return;
    NfcManager.instance.stopSession();
    setState(() => _isScanning = false);
  }

  String _bytesToHex(List<int> bytes) =>
      bytes.map((b) => b.toRadixString(16).padLeft(2, '0').toUpperCase()).join('');

  // ── UI ────────────────────────────────────────────────

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(
        child: SingleChildScrollView(
          padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              _buildHeader(),
              const SizedBox(height: 24),
              _buildConnectionCard(),
              const SizedBox(height: 20),
              _buildScannerCard(),
              const SizedBox(height: 20),
              if (_scanHistory.isNotEmpty) _buildHistoryCard(),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildHeader() {
    return Row(
      children: [
        Container(
          padding: const EdgeInsets.all(10),
          decoration: BoxDecoration(
            gradient: const LinearGradient(
              colors: [Color(0xFF6C63FF), Color(0xFF00D9F5)],
            ),
            borderRadius: BorderRadius.circular(14),
          ),
          child: const Icon(Icons.nfc, size: 28, color: Colors.white),
        ),
        const SizedBox(width: 14),
        Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('RFID Scanner', style: GoogleFonts.inter(fontSize: 22, fontWeight: FontWeight.w700)),
            Text('Scan & Publish via MQTT', style: GoogleFonts.inter(fontSize: 13, color: Colors.white54)),
          ],
        ),
        const Spacer(),
        _StatusDot(isConnected: _isConnected),
      ],
    );
  }

  Widget _buildConnectionCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Icon(Icons.cloud_outlined, color: Color(0xFF6C63FF), size: 20),
              const SizedBox(width: 8),
              Text('MQTT Broker', style: GoogleFonts.inter(fontSize: 15, fontWeight: FontWeight.w600)),
            ],
          ),
          const SizedBox(height: 16),
          Row(
            children: [
              Expanded(
                flex: 3,
                child: TextField(
                  controller: _ipController,
                  enabled: !_isConnected,
                  style: const TextStyle(fontSize: 14),
                  decoration: const InputDecoration(labelText: 'IP Address', prefixIcon: Icon(Icons.dns_outlined, size: 20)),
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                flex: 1,
                child: TextField(
                  controller: _portController,
                  enabled: !_isConnected,
                  keyboardType: TextInputType.number,
                  style: const TextStyle(fontSize: 14),
                  decoration: const InputDecoration(labelText: 'Port'),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          SizedBox(
            width: double.infinity,
            height: 50,
            child: AnimatedSwitcher(
              duration: const Duration(milliseconds: 300),
              child: _isConnected
                  ? ElevatedButton.icon(
                      key: const ValueKey('disconnect'),
                      onPressed: _disconnectMqtt,
                      icon: const Icon(Icons.link_off),
                      label: const Text('Disconnect'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.redAccent.withValues(alpha: 0.2),
                        foregroundColor: Colors.redAccent,
                        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                      ),
                    )
                  : ElevatedButton.icon(
                      key: const ValueKey('connect'),
                      onPressed: _isConnecting ? null : _connectMqtt,
                      icon: _isConnecting
                          ? const SizedBox(width: 18, height: 18, child: CircularProgressIndicator(strokeWidth: 2))
                          : const Icon(Icons.link),
                      label: Text(_isConnecting ? 'Connecting…' : 'Connect'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFF6C63FF),
                        foregroundColor: Colors.white,
                        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                      ),
                    ),
            ),
          ),
          const SizedBox(height: 10),
          Text(_statusMessage, style: GoogleFonts.inter(fontSize: 12, color: Colors.white38)),
        ],
      ),
    );
  }

  Widget _buildScannerCard() {
    return _GlassCard(
      child: Column(
        children: [
          if (!_nfcAvailable)
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.redAccent.withValues(alpha: 0.1),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Row(
                children: [
                  const Icon(Icons.error_outline, color: Colors.redAccent),
                  const SizedBox(width: 12),
                  Expanded(
                    child: Text('NFC is not available on this device',
                        style: GoogleFonts.inter(color: Colors.redAccent, fontSize: 13)),
                  ),
                ],
              ),
            )
          else ...[
            // Scan animation area
            AnimatedBuilder(
              animation: _pulseAnimation,
              builder: (context, child) {
                return Container(
                  width: 140,
                  height: 140,
                  decoration: BoxDecoration(
                    shape: BoxShape.circle,
                    gradient: RadialGradient(
                      colors: _isScanning
                          ? [
                              const Color(0xFF6C63FF).withValues(alpha: 0.3 * _pulseAnimation.value),
                              const Color(0xFF6C63FF).withValues(alpha: 0.05),
                            ]
                          : [Colors.white10, Colors.white.withValues(alpha: 0.02)],
                    ),
                    border: Border.all(
                      color: _isScanning ? const Color(0xFF6C63FF).withValues(alpha: 0.5) : Colors.white12,
                      width: 2,
                    ),
                  ),
                  child: Icon(
                    _isScanning ? Icons.contactless : Icons.nfc,
                    size: 56,
                    color: _isScanning ? const Color(0xFF6C63FF) : Colors.white30,
                  ),
                );
              },
            ),
            const SizedBox(height: 16),
            Text(
              _isScanning ? 'Hold your card near the phone…' : 'Tap Start to begin scanning',
              style: GoogleFonts.inter(fontSize: 14, color: Colors.white54),
            ),
            if (_lastTag.isNotEmpty) ...[
              const SizedBox(height: 12),
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
                decoration: BoxDecoration(
                  color: const Color(0xFF00D9F5).withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(10),
                  border: Border.all(color: const Color(0xFF00D9F5).withValues(alpha: 0.3)),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    const Icon(Icons.badge_outlined, color: Color(0xFF00D9F5), size: 18),
                    const SizedBox(width: 8),
                    Flexible(
                      child: Text(_lastTag,
                          style: GoogleFonts.firaCode(color: const Color(0xFF00D9F5), fontSize: 14)),
                    ),
                  ],
                ),
              ),
            ],
            const SizedBox(height: 20),
            SizedBox(
              width: double.infinity,
              height: 50,
              child: ElevatedButton.icon(
                onPressed: !_isConnected
                    ? null
                    : (_isScanning ? _stopScanning : _startScanning),
                icon: Icon(_isScanning ? Icons.stop : Icons.play_arrow),
                label: Text(_isScanning ? 'Stop Scanning' : 'Start Scanning'),
                style: ElevatedButton.styleFrom(
                  backgroundColor: _isScanning ? Colors.redAccent : const Color(0xFF00D9F5),
                  foregroundColor: _isScanning ? Colors.white : const Color(0xFF0A0E21),
                  disabledBackgroundColor: Colors.white10,
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                ),
              ),
            ),
            if (!_isConnected)
              Padding(
                padding: const EdgeInsets.only(top: 8),
                child: Text('Connect to MQTT broker first',
                    textAlign: TextAlign.center,
                    style: GoogleFonts.inter(fontSize: 11, color: Colors.white30)),
              ),
          ],
        ],
      ),
    );
  }

  Widget _buildHistoryCard() {
    return _GlassCard(
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              const Icon(Icons.history, color: Color(0xFF00D9F5), size: 20),
              const SizedBox(width: 8),
              Text('Scan History', style: GoogleFonts.inter(fontSize: 15, fontWeight: FontWeight.w600)),
              const Spacer(),
              Text('Topic: $_topic', style: GoogleFonts.inter(fontSize: 10, color: Colors.white30)),
            ],
          ),
          const SizedBox(height: 12),
          ..._scanHistory.take(10).map((log) => Padding(
                padding: const EdgeInsets.only(bottom: 8),
                child: Row(
                  children: [
                    const Icon(Icons.chevron_right, size: 16, color: Color(0xFF6C63FF)),
                    const SizedBox(width: 6),
                    Expanded(
                      child: Text(log.tag, style: GoogleFonts.firaCode(fontSize: 12, color: Colors.white70)),
                    ),
                    Text(
                      '${log.time.hour.toString().padLeft(2, '0')}:${log.time.minute.toString().padLeft(2, '0')}:${log.time.second.toString().padLeft(2, '0')}',
                      style: GoogleFonts.inter(fontSize: 11, color: Colors.white30),
                    ),
                  ],
                ),
              )),
        ],
      ),
    );
  }
}

// ── Reusable Widgets ──────────────────────────────────

class _GlassCard extends StatelessWidget {
  final Widget child;
  const _GlassCard({required this.child});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.all(20),
      decoration: BoxDecoration(
        color: const Color(0xFF1A1F38).withValues(alpha: 0.7),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(color: Colors.white.withValues(alpha: 0.06)),
        boxShadow: [
          BoxShadow(color: Colors.black.withValues(alpha: 0.3), blurRadius: 20, offset: const Offset(0, 8)),
        ],
      ),
      child: child,
    );
  }
}

class _StatusDot extends StatelessWidget {
  final bool isConnected;
  const _StatusDot({required this.isConnected});

  @override
  Widget build(BuildContext context) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
      decoration: BoxDecoration(
        color: (isConnected ? Colors.greenAccent : Colors.redAccent).withValues(alpha: 0.15),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 8,
            height: 8,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: isConnected ? Colors.greenAccent : Colors.redAccent,
            ),
          ),
          const SizedBox(width: 6),
          Text(isConnected ? 'Online' : 'Offline',
              style: GoogleFonts.inter(
                  fontSize: 11, color: isConnected ? Colors.greenAccent : Colors.redAccent)),
        ],
      ),
    );
  }
}

class _ScanLog {
  final String tag;
  final DateTime time;
  _ScanLog({required this.tag, required this.time});
}
