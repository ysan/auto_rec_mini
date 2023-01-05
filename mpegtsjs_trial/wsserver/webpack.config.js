const path = require('path');

module.exports = {
  mode: 'development',
  entry: path.resolve(__dirname, 'src/index.js'),
  output: {
    path: path.resolve(__dirname, 'dist'),
    filename: 'bundle.js'
  },
  target: 'node',
  externals: {
    bufferutil: 'bufferutil',
    'utf-8-validate': 'utf-8-validate'
  },
  resolve: {
    alias: {
      'ws': path.resolve(__dirname, 'node_modules/ws/index.js')
    }
  }
};
