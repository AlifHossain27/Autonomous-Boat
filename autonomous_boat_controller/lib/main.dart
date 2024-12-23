import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';
import 'package:permission_handler/permission_handler.dart';

void main() => runApp(MyApp());

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false, 
      home: BluetoothScreen(),
    );
  }
}

class BluetoothScreen extends StatefulWidget {
  @override
  _BluetoothScreenState createState() => _BluetoothScreenState();
}

class _BluetoothScreenState extends State<BluetoothScreen> {
  BluetoothDevice? connectedDevice;
  BluetoothCharacteristic? targetCharacteristic;
  List<String> receivedData = [];
  final ScrollController _scrollController = ScrollController();

  final TextEditingController latitudeController = TextEditingController();
  final TextEditingController longitudeController = TextEditingController();

  bool isExpanded = true; 

  @override
  void initState() {
    super.initState();
    requestBluetoothPermissions();
    checkBluetoothAvailability();
    scanForDevices();
  }

  void requestBluetoothPermissions() async {
    PermissionStatus bluetoothConnectStatus = await Permission.bluetoothConnect.request();
    PermissionStatus bluetoothScanStatus = await Permission.bluetoothScan.request();

    if (bluetoothConnectStatus.isGranted && bluetoothScanStatus.isGranted) {
      print("Bluetooth permissions granted.");
    } else {
      print("Bluetooth permissions denied.");
    }
  }

  void checkBluetoothAvailability() async {
    final FlutterBlue flutterBlue = FlutterBlue.instance;

    bool isAvailable = await flutterBlue.isAvailable;
    bool isOn = await flutterBlue.isOn;

    if (!isAvailable) {
      print("Bluetooth is not available.");
    } else if (!isOn) {
      print("Bluetooth is not turned on. Please enable Bluetooth.");
    } else {
      print("Bluetooth is available and turned on.");
    }
  }

  void scanForDevices() async {
    final FlutterBlue flutterBlue = FlutterBlue.instance;
    flutterBlue.startScan(timeout: Duration(seconds: 4));

    flutterBlue.scanResults.listen((results) {
      for (ScanResult result in results) {
        print('Device found: ${result.device.name}');
        if (result.device.name == 'HC-05') {
          print('HC-05 found, connecting...');
          connectToDevice(result.device);
        }
      }
    });
  }

  void connectToDevice(BluetoothDevice device) async {
    try {
      await device.connect();
      connectedDevice = device;
      print("Connected to device: ${device.name}");
      var services = await device.discoverServices();
      for (var service in services) {
        for (var characteristic in service.characteristics) {
          if (characteristic.properties.write) {
            targetCharacteristic = characteristic;
            print("Target characteristic found.");
          }
          if (characteristic.properties.notify) {
            await characteristic.setNotifyValue(true);
            characteristic.value.listen((value) {
              setState(() {
                receivedData.add(String.fromCharCodes(value));
              });
              _scrollToBottom();
            });
          }
        }
      }
      setState(() {});
    } catch (e) {
      print("Error connecting to device: $e");
    }
  }

  bool isValidCoordinates() {
    double? lat = double.tryParse(latitudeController.text);
    double? lon = double.tryParse(longitudeController.text);
    return lat != null &&
        lon != null &&
        lat >= -90 &&
        lat <= 90 &&
        lon >= -180 &&
        lon <= 180;
  }

  void sendCoordinates() async {
    if (targetCharacteristic != null) {
      if (isValidCoordinates()) {
        String message = '${latitudeController.text},${longitudeController.text}';
        try {
          await targetCharacteristic?.write(message.codeUnits);
          print("Sent: $message");
          FocusScope.of(context).unfocus();
        } catch (e) {
          print("Error sending data: $e");
        }
      } else {
        print("Invalid coordinates. Please check input.");
      }
    } else {
      print("Bluetooth not connected.");
    }
  }

  void _scrollToBottom() {
    if (_scrollController.hasClients) {
      _scrollController.jumpTo(_scrollController.position.maxScrollExtent);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text("Autonomous Boat Controller"),
        backgroundColor: Colors.blueAccent, 
      ),
      body: SingleChildScrollView( 
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              SizedBox(height: 32),
              TextField(
                controller: latitudeController,
                decoration: InputDecoration(
                  hintText: "Enter Latitude",
                  labelText: "Latitude",
                  filled: true,
                  fillColor: Colors.blue[50],
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12.0),
                    borderSide: BorderSide.none,
                  ),
                  contentPadding: EdgeInsets.symmetric(vertical: 16, horizontal: 16),
                ),
                keyboardType: TextInputType.number,
              ),
              SizedBox(height: 16),
              TextField(
                controller: longitudeController,
                decoration: InputDecoration(
                  hintText: "Enter Longitude",
                  labelText: "Longitude",
                  filled: true,
                  fillColor: Colors.blue[50],
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(12.0),
                    borderSide: BorderSide.none,
                  ),
                  contentPadding: EdgeInsets.symmetric(vertical: 16, horizontal: 16),
                ),
                keyboardType: TextInputType.number,
              ),
              SizedBox(height: 16),
              ElevatedButton(
                onPressed: sendCoordinates,
                child: Text("Send Coordinates"),
                style: ElevatedButton.styleFrom(
                  minimumSize: Size(double.infinity, 50),
                  backgroundColor: Colors.blueAccent, 
                  textStyle: TextStyle(fontSize: 16),
                ),
              ),
              SizedBox(height: 16),
              if (connectedDevice != null)
                Padding(
                  padding: const EdgeInsets.only(top: 16.0),
                  child: Text("Bluetooth Connected: ${connectedDevice?.name}"),
                )
              else
                Padding(
                  padding: const EdgeInsets.only(top: 16.0),
                  child: Text("Connecting to Bluetooth..."),
                ),
              SizedBox(height: 16),
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  Text("Monitor:"),
                  IconButton(
                    onPressed: () {
                      setState(() {
                        isExpanded = !isExpanded; 
                      });
                    },
                    icon: Icon(
                      isExpanded ? Icons.expand_more : Icons.expand_less,
                      color: Colors.blueAccent, 
                    ),
                  ),
                ],
              ),
              Container(
                decoration: BoxDecoration(
                  color: Colors.blue[50], 
                  borderRadius: BorderRadius.circular(12), 
                ),
                child: AnimatedContainer(
                  duration: Duration(milliseconds: 300), 
                  height: isExpanded ? 250 : 0,
                  child: ListView.builder(
                    controller: _scrollController, 
                    shrinkWrap: true, 
                    itemCount: receivedData.length,
                    itemBuilder: (context, index) {
                      return Padding(
                        padding: const EdgeInsets.symmetric(vertical: 0.0, horizontal: 10.0),
                        child: Text(
                          receivedData[index],
                          style: TextStyle(fontSize: 16, fontFamily: 'Courier'),
                        ),
                      );
                    },
                  ),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
