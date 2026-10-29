const fs = require('node:fs');
const crypto = require('node:crypto');
const vm = require('node:vm');
const assert = require('node:assert/strict');

const archivePath = process.argv[2];
if (!archivePath) throw new Error('Expected the Linsoft Browser app.asar path.');
const archive = fs.readFileSync(archivePath);
const headerSize = archive.readUInt32LE(4);
const jsonSize = archive.readUInt32LE(12);
assert.equal(archive.readUInt32LE(0), 4, 'Invalid ASAR size header');
assert.ok(jsonSize <= headerSize - 8, 'Invalid ASAR JSON size');
const header = JSON.parse(archive.subarray(16, 16 + jsonSize).toString('utf8'));
const entry = header.files['main.cjs'];
assert.ok(entry && !entry.unpacked, 'Expected a packed main.cjs');
const payload = archive.subarray(8 + headerSize);
const original = payload.subarray(Number(entry.offset), Number(entry.offset) + entry.size);
assert.equal(original.length, entry.size, 'Truncated main.cjs');
if (entry.integrity) {
  assert.equal(crypto.createHash('sha256').update(original).digest('hex'), entry.integrity.hash);
}
let source = original.toString('utf8');
const marker = '// Linsoft OS: pass validated external web URLs to the browser.';
if (source.includes(marker)) {
  console.log('Linsoft Browser external URL fix is already installed.');
  process.exit(0);
}
const argumentLine = '  const explicitUrl = args.find((value) => /^linsoft:\\/\\/(?:apps|centrum|install\\?|uninstall\\?)/i.test(value) || /^file:\\/\\//i.test(value));';
const dispatchLine = "    if (safeInitialUrl || /^https?:\\/\\//i.test(normalizedInitialUrl) === false) dispatchExternalUrl(window, normalizedInitialUrl || 'linsoft://start');";
assert.equal(source.split(argumentLine).length, 2, 'Unexpected Browser argument parser');
assert.equal(source.split(dispatchLine).length, 2, 'Unexpected Browser startup dispatch');
source = source.replace(argumentLine, `${marker}\n${argumentLine.replace('args.find((value) => ', 'args.find((value) => supportedWebUrl(value) || ')}`);
source = source.replace(dispatchLine, "    if (safeInitialUrl || supportedWebUrl(normalizedInitialUrl)) dispatchExternalUrl(window, normalizedInitialUrl || 'linsoft://start');");
new vm.Script(source, { filename: 'main.cjs' });

const parser = source.match(/function externalUrlFromArgs\(args\) \{[\s\S]*?\n\}/);
assert.ok(parser, 'Missing Browser argument parser');
const context = vm.createContext({
  supportedWebUrl: value => /^https?:\/\/[^/\s]+(?:\/|$)/i.test(value),
  path: require('node:path'),
  pathToFileURL: require('node:url').pathToFileURL,
});
vm.runInContext(`${parser[0]}; this.parse = externalUrlFromArgs;`, context);
for (const url of ['https://linsoft1.github.io/linsoft-os/', 'http://example.org/', 'file:///tmp/test.html', 'linsoft://apps']) {
  assert.equal(context.parse(['linsoft-browser', url]), url);
}
assert.equal(context.parse(['linsoft-browser', '--no-sandbox']), undefined);
assert.equal(context.parse(['linsoft-browser', 'javascript:alert(1)']), undefined);

const replacement = Buffer.from(source);
const hash = data => crypto.createHash('sha256').update(data).digest('hex');
const blockSize = entry.integrity?.blockSize || 4194304;
const blocks = [];
for (let offset = 0; offset < replacement.length; offset += blockSize) {
  blocks.push(hash(replacement.subarray(offset, offset + blockSize)));
}
entry.offset = String(payload.length);
entry.size = replacement.length;
entry.integrity = { algorithm: 'SHA256', hash: hash(replacement), blockSize, blocks };
const json = Buffer.from(JSON.stringify(header));
const paddedSize = Math.ceil(json.length / 4) * 4;
const newHeader = Buffer.alloc(8 + paddedSize);
newHeader.writeUInt32LE(4 + paddedSize, 0);
newHeader.writeUInt32LE(json.length, 4);
json.copy(newHeader, 8);
const sizeHeader = Buffer.alloc(8);
sizeHeader.writeUInt32LE(4, 0);
sizeHeader.writeUInt32LE(newHeader.length, 4);
const temporary = `${archivePath}.linsoft-tmp`;
fs.writeFileSync(temporary, Buffer.concat([sizeHeader, newHeader, payload, replacement]), { mode: fs.statSync(archivePath).mode });
fs.renameSync(temporary, archivePath);
console.log('Fixed Linsoft Browser startup and existing-window HTTP/HTTPS handling.');
