# Node.js JPEG turbo scaler module

Native C++ Node.js module which uses [libjpeg-turbo](http://libjpeg-turbo.virtualgl.org) for extremely fast JPEG compression and decompression.


### Installation

The module have been tested to build correctly only on OS X and requires libjpeg-turbo to be installed manually via [Homebrew](http://brew.sh/). It should be pretty easy to build it on any system, the only dependency is libjpeg-turbo library which you can easily install using your favorite package manager and simply add include and lib directories to `bindings.gyp` file.

Installation sequence:

* Install libjpeg-turbo
```bash
brew install jpeg-turbo
```
* Install the module using npm
```bash
npm install jpeg-turbo-scaler
```

### Usage

*function* **decompress**(`path`, `targetWidth`, `targetHeight`, `callback`)

Reads a JPEG file referenced by `path` from local file system and resizes it to fit into `targetWidth`x`targetHeight` box keeping original aspect ratio.
arguments:

`path`:  a string containing JPEG image file path (absolute or relative).

`targetWidth` and `targetHeight`:  desired dimensions of decompressed image.

`callback`:  *function*(`bitmap`, `width`, `height`) where `bitmap` is a Buffer with RGBA pixels data.

*function* **scale**(`path`, `targetWidth`, `targetHeight`, `callback`)

Reads a JPEG file referenced by `path` from local file system, resizes it to fit into `targetWidth`x`targetHeight` box keeping original aspect ratio, and compresses the result to JPEG.
arguments:

`path`:  a string containing JPEG image file path (absolute or relative).

`targetWidth` and `targetHeight`:  desired dimensions of decompressed image.

`callback`:  *function*(`jpeg`, `width`, `height`) where `bitmap` is a JPEG file data.

### Example
```javascript
var jpeg = require('jpeg-turbo-scaler');
jpeg.decompress('image.jpg', 300, 300, function(bitmap, width, height) {
  console.log('Got ' + bitmap.length + ' bytes RGBA bitmap');
});

jpeg.scale('image.jpg', 300, 300, function(jpeg, width, height) {
  console.log('Got ' + jpeg.length + ' scaled JPEG file');
  require('fs').writeFileSync('output.jpg', jpeg);
});
```
