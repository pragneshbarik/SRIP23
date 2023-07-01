import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:gait_client/server_input.dart';
import 'package:sensors_plus/sensors_plus.dart';
import 'package:dio/dio.dart';
import 'dart:async';
import 'dart:core';

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
  final Stopwatch _watch = Stopwatch();

  Timer? dataTimer;
  int packetSize = 200;
  double _samplingRate = 0;
  String _gyroX = "";
  String _gyroY = "";
  String _gyroZ = "";
  String _accX = "";
  String _accY = "";
  String _accZ = "";
  final List<int> _timeStamps = [];
  int _packetSent = 0;
  final uriController = TextEditingController();
  List<String> _currRead = [];
  List<String> gaitClass = [
    "Walking",
    "Fast Walking",
    "Running",
    "Stair Up",
    "Stair Down",
    "Stationary"
  ];

  String _selectedClass = '';
  bool start = false;

  void setValue(String value) {
    setState(() {
      _selectedClass = value;
    });
  }

  final List<List<String>> _prevReads = [];
  String _predictedClass = "Stationary";

  @override
  void initState() {
    super.initState();
    // int eventCount = 0;
    DateTime startTime = DateTime.now();
    uriController.text = "http://10.7.12.28:5000";
    _selectedClass = gaitClass.first;
    gyroscopeEvents.listen((GyroscopeEvent event) {
      // eventCount++;
      // Duration elapsedTime = DateTime.now().difference(startTime);
      // _samplingRate = eventCount / (elapsedTime.inMilliseconds) / 1000;

      setState(() {
        // _samplingRate = _samplingRate;
        _gyroX = event.x.toStringAsFixed(5);
        _gyroY = event.y.toStringAsFixed(5);
        _gyroZ = event.z.toStringAsFixed(5);
        _currRead = [_gyroX, _gyroY, _gyroZ];
      });
      // eventCount = 0;
    });
    accelerometerEvents.listen((AccelerometerEvent event) {
      setState(() {
        _accX = event.x.toStringAsFixed(5);
        _accY = event.y.toStringAsFixed(5);
        _accZ = event.z.toStringAsFixed(5);
        _currRead.addAll([_accX, _accY, _accZ]);
        if (start) {
          _timeStamps.add(_watch.elapsedMilliseconds);
          _prevReads.add(_currRead);
        }
        if ((_prevReads.length == packetSize) && start) {
          setState(() {
            _packetSent += 1;
            _samplingRate = (200 / (_timeStamps[199] - _timeStamps[0])) * 1000;
          });

          saveDataToServer(
            uriController.text,
            jsonEncode({
              "image": _prevReads,
              "timestamps": _timeStamps,
              "label": _selectedClass
            }),
          ).then((value) {
            if (value != null) {
              setState(() {
                _predictedClass = value["prediction"];
              });
              print(value);
            }
          });

          _prevReads.clear();
          _timeStamps.clear();
          _watch.reset();
        }
      });
    });
  }

  @override
  void dispose() {
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
            Column(
              children: [
                ServerInput(uriController, !start),
                Text("Packets Sent: ${_packetSent}"),
                Text("Predicted Class: ${_predictedClass}"),
                Text("Sampling Rate: ${_samplingRate}")
              ],
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
                if (start) _watch.start();
              },
              child: Text(start ? "Stop Logging" : "Start Logging"),
            ),
          ],
        ),
      ),
    );
  }
}

Future<dynamic> saveDataToServer(String uri, dynamic data) async {
  final dio = Dio();
  try {
    final response = await dio.post(
      '$uri/log', // Replace with your Flask server endpoint
      data: data,
    );

    if (response.statusCode == 200) {
      print('Data sent to the server');
      return response.data;
    } else {
      print('Failed to send data to the server');
    }
  } catch (e) {
    print('Error occurred: $e');
  }
  return null;
}

Future<String> predictGait(String uri, dynamic data) async {
  final dio = Dio();
  print(data);
  try {
    final response = await dio.post(
      '${uri}/predict', // Replace with your Flask server endpoint
      data: data,
    );

    if (response.statusCode == 200) {
      print('Data sent to the server');
      print(response.data);
      return response.data;
    } else {
      print('Failed to send data to the server');
    }
  } catch (e) {
    print('Error occurred: $e');
    // return false;
  }
  return "Error";
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
