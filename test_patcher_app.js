// node test_patcher_app.js <app .html> <original v1.0 xbe> <original v1.5 xbe> <patch_xbe.py v1.0 out> <TU Skip out> <v1.5 out>
// The app's patching must give exactly what patch_xbe.py gives, for all three builds.
const fs = require('fs'), vm = require('vm'), crypto = require('crypto');
const [app, v10, v15, o10, o10tu, o15] = process.argv.slice(2);
const ctx = { module: { exports: {} }, Buffer };
vm.createContext(ctx); vm.runInContext(fs.readFileSync(app, 'utf8').match(/<script id="core">([\s\S]*?)<\/script>/)[1], ctx);
const { sgPatch, sgSha1 } = ctx.module.exports;
for (const n of [0, 55, 56, 64, 65, 100001]) { const b = crypto.randomBytes(n);
  if (Buffer.from(sgSha1(new Uint8Array(b))).toString('hex') !== crypto.createHash('sha1').update(b).digest('hex')) throw new Error('SHA-1 wrong at ' + n); }
for (const [src, v, tu, want] of [[v10, 'v1.0', false, o10], [v10, 'v1.0', true, o10tu], [v15, 'v1.5', false, o15]]) {
  const y = Buffer.from(sgPatch(new Uint8Array(fs.readFileSync(src)), v, tu));
  if (Buffer.compare(y, fs.readFileSync(want))) { console.error(v + (tu ? ' TU Skip' : '') + ': DIFFERENT from patch_xbe.py'); process.exit(1); }
  console.log(v + (tu ? ' TU Skip' : '') + ': identical to patch_xbe.py');
}
