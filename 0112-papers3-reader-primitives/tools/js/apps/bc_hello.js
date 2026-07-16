// Bytecode acceptance app (task ibe5): authored on the host, compiled by
// tools/js/s3jsc.c to relocated 32-bit bytecode, embedded in firmware,
// executed on device WITHOUT the parser. ES5-stricter dialect.
s3.reset();
var header = s3.col().pad(16, 40, 4, 40).gap(8)
  .add(s3.text('JS bytecode app').font(s3.FONT_UI), s3.divider(2, 0));
var content = s3.col().pad(24, 40, 24, 40).gap(16).add(
  s3.text('This screen was compiled on the'),
  s3.text('host and shipped as bytecode.'),
  s3.text('No parser ran on the device.').center().gray(96),
  s3.progressBar(1000, 24).height(24));
var footer = s3.col().pad(6, 40, 10, 40).gap(6)
  .add(s3.divider(1, 0),
       s3.text('authoring pipeline: js -> s3jsc -> C header -> EPD')
         .font(s3.FONT_UI).gray(96));
print('bc_hello: rendering, abi v' + s3Version());
s3.render({header: header, content: content, footer: footer, full: true});
