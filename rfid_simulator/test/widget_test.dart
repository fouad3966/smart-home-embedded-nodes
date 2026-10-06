import 'package:flutter_test/flutter_test.dart';
import 'package:rfid_simulator/main.dart';

void main() {
  testWidgets('App renders', (WidgetTester tester) async {
    await tester.pumpWidget(const RfidScannerApp());
    expect(find.text('RFID Scanner'), findsOneWidget);
  });
}
