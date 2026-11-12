// node test_patcher_emu.js <app .html> <release xefu1_1.xex> <original xefu1_1.xex> [<another original> ...]
// The page's emulator patching, run outside a browser: every original must give the release exactly; wrong files are refused.
const fs=require('fs'),vm=require('vm'),crypto=require('crypto');
const [app,r89,...origs]=process.argv.slice(2);
const ctx={module:{exports:{}},Buffer}; vm.createContext(ctx);
vm.runInContext(fs.readFileSync(app,'utf8').match(/<script id="core">([\s\S]*?)<\/script>/)[1],ctx);
const {sgEmuDetect,sgEmuPatch,SGE}=ctx.module.exports;
const want=fs.readFileSync(r89), sha=b=>crypto.createHash('sha1').update(b).digest('hex');
for (const p of origs) {
  const x=new Uint8Array(fs.readFileSync(p)); const t0=Date.now();
  const d=sgEmuDetect(x); if (d.error) { console.error(p+': '+d.error); process.exit(1); }
  const y=Buffer.from(sgEmuPatch(x));
  if (Buffer.compare(y,want)) { console.error(p+': DIFFERENT from the release'); process.exit(1); }
  console.log(p.split('/').pop()+': recognised as "'+d.name+'"; unpacked program sha1 '+sha(Buffer.from(d.image)).slice(0,16)+'...; patched file identical to '+SGE.name+' (sha1 '+sha(y)+'), '+(Date.now()-t0)+' ms');
}
function refuse(label,buf){ const d=sgEmuDetect(new Uint8Array(buf)); if(!d.error){console.error(label+': ACCEPTED (wrong)');process.exit(1);} console.log(label+': refused - '+d.error.slice(0,120)); }
refuse('the release itself given as an original',want);
const m=Buffer.from(fs.readFileSync(origs[0])); m[0x40000]^=0x55; refuse('an original with one byte of its compressed data changed',m);
refuse('a file that is not an XEX',Buffer.from('XBEH'+'x'.repeat(5000)));
refuse('an empty file',Buffer.alloc(0));
