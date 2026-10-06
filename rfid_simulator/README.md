# rfid_simulator

this is a simple rfid simulator for testing purposes. It can be used to simulate the behavior of an rfid reader and tag.    


## Usage
1. Clone the repository
2. Install the dependencies
```bashpip install -r requirements.txt
```
3. Run the simulator        
4. Use the provided API to interact with the simulator
```bashpython main.py
```
## API
- `POST /read`: Simulate reading an rfid tag. The request body should contain the tag id and the response will contain the tag data.
- `POST /write`: Simulate writing to an rfid tag. The request body should contain the tag id and the data to be written. The response will indicate whether the write was successful or not.
- `GET /tags`: Get a list of all the tags currently in the simulator. The response will contain an array of tag ids and their corresponding data.

