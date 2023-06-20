import 'package:flutter/material.dart';
import 'package:sensors_plus/sensors_plus.dart';
import 'package:dio/dio.dart';
import 'dart:async';
import 'dart:convert';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Flutter Demo',
      theme: ThemeData(
        primarySwatch: Colors.blueGrey,
      ),
      home: const MyHomePage(title: 'Gait Classifier'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({Key? key, required this.title}) : super(key: key);

  final String title;

  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  Stopwatch watch = Stopwatch();

  Timer? dataTimer;
  int packetSize = 120;
  String _gyroX = "";
  String _gyroY = "";
  String _gyroZ = "";
  String _accX = "";
  String _accY = "";
  String _accZ = "";
  List<String> currRead = [];
  List<String> gaitClass = [
    "Walking",
    "Running",
    "Stair Up",
    "Stair Down",
    "Stationary"
  ];

  String selectedClass = '';
  bool start = false;

  void setValue(String value) {
    setState(() {
      selectedClass = value;
    });
  }

  List<List<String>> prevReads = [];

  @override
  void initState() {
    super.initState();
    selectedClass = gaitClass.first;
    gyroscopeEvents.listen((GyroscopeEvent event) {
      setState(() {
        _gyroX = event.x.toStringAsFixed(5);
        _gyroY = event.y.toStringAsFixed(5);
        _gyroZ = event.z.toStringAsFixed(5);
        currRead = [_gyroX, _gyroY, _gyroZ];
      });
    });
    accelerometerEvents.listen((AccelerometerEvent event) {
      setState(() {
        _accX = event.x.toStringAsFixed(5);
        _accY = event.y.toStringAsFixed(5);
        _accZ = event.z.toStringAsFixed(5);
        currRead.addAll([_accX, _accY, _accZ]);
        if (start) {
          currRead.add(watch.elapsedMilliseconds.toString());
          // currRead.add(dropDownValue);
          prevReads.add(currRead);
        }

        if (prevReads.length == packetSize) {
          sendDataToServer(
              jsonEncode({"image": prevReads, "label": selectedClass}));
          prevReads.clear();
        }
      });
    });

    dataTimer = Timer.periodic(const Duration(milliseconds: 100), (_) {});
  }

  @override
  void dispose() {
    dataTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.spaceAround,
          children: <Widget>[
            const Text(
              'Sensor Data',
              style: TextStyle(fontSize: 32),
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceAround,
              children: [
                Column(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      'gx: $_gyroX',
                    ),
                    Text(
                      'gy: $_gyroY',
                    ),
                    Text(
                      'gz: $_gyroZ',
                    ),
                  ],
                ),
                Column(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      'ax: $_accX',
                    ),
                    Text(
                      'ay: $_accY',
                    ),
                    Text(
                      'az: $_accZ',
                    ),
                  ],
                ),
              ],
            ),
            DropdownButtonExample(list: gaitClass, setValue: setValue),
            OutlinedButton(
              onPressed: () {
                start = !start;
                if (start) {
                  watch.start();
                } else {
                  watch.stop();
                }
              },
              child: Text(start ? "Stop Logging" : "Start Logging"),
            ),
          ],
        ),
      ),
    );
  }
}

void sendDataToServer(dynamic data) async {
  final dio = Dio();
  try {
    final response = await dio.post(
      'http://10.7.12.28:5000/log', // Replace with your Flask server endpoint
      data: data,
    );

    if (response.statusCode == 200) {
      print('Data sent to the server');
    } else {
      print('Failed to send data to the server');
    }
  } catch (e) {
    print('Error occurred: $e');
  }
}

class DropdownButtonExample extends StatefulWidget {
  final List<String> list;
  final Function setValue;

  const DropdownButtonExample(
      {Key? key, required this.list, required this.setValue})
      : super(key: key);

  @override
  _DropdownButtonExampleState createState() => _DropdownButtonExampleState();
}

class _DropdownButtonExampleState extends State<DropdownButtonExample> {
  late String dropdownValue;

  @override
  void initState() {
    super.initState();
    dropdownValue = widget.list.first;
  }

  @override
  Widget build(BuildContext context) {
    return DropdownButton<String>(
      value: dropdownValue,
      icon: const Icon(Icons.arrow_downward),
      elevation: 16,
      style: const TextStyle(color: Colors.deepPurple),
      underline: Container(
        height: 2,
        color: Colors.deepPurpleAccent,
      ),
      onChanged: (String? value) {
        if (value != null) {
          setState(() {
            dropdownValue = value;
          });
          widget.setValue(value);
        }
      },
      items: widget.list.map<DropdownMenuItem<String>>((String value) {
        return DropdownMenuItem<String>(
          value: value,
          child: Text(value),
        );
      }).toList(),
    );
  }
}
