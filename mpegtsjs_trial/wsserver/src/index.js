import ws from 'ws';
import * as child_process from 'child_process';
import * as process from 'process';

console.log('server begin');

let command = '';
const commandArgs = [];
for (let i = 0; i < process.argv.length; ++i) {
  console.log(i + ': ' + process.argv[i]);
  if (i == 2) {
    command = process.argv[i];
  } else if (i >= 3) {
    commandArgs.push(process.argv[i]);
  }
}

const wss = new ws.Server({ port: 50000 });

wss.on('connection', function connection(ws, request) {
  //console.log('ws: ', ws);
  //console.log('request: ', request);

  const proc = child_process.spawn(command, commandArgs);
  console.log('child:' + proc.pid);
  proc.stdout.on('data', (data) => {
    ws.send(data);
  });
  proc.stderr.on('data', (data) => {
    console.log(data.toString());
  });
});
