let chromium; try { chromium = require('playwright').chromium; } catch (e) { chromium = require('/opt/npm-tools/node_modules/playwright').chromium; }
const fs = require('fs'), crypto = require('crypto');
// node test_patcher_page.js <app .html> <original v1.0 xbe> <original v1.5 xbe> <v1.0 out> <TU Skip out> <v1.5 out> <release xefu1_1.xex> <original xefu1_1.xex> <the 128 MB original>
// The page itself, clicked through in a real browser (Playwright's Chromium): the 360 tick box, the second file button, the
// folder save, the question before replacing, the fall-backs. The two pickers are stood in for (a page cannot drive the
// real dialogs); everything else is the page's own code. Screenshots go to the system's temp folder.
const [APP, X10, X15, O10, O10TU, O15, R89, E64, E128] = process.argv.slice(2);
const PAGE = 'file://' + require('path').resolve(APP);
const sha = p => crypto.createHash('sha1').update(fs.readFileSync(p)).digest('hex');
const want = { tu: sha(O10TU), v10: sha(O10), v15: sha(O15), emu: sha(R89) };
let ok = true; const say = (name, good, extra) => { ok = ok && !!good; console.log((good ? 'ok   ' : 'FAIL ') + name + (extra && !good ? '  [' + extra + ']' : '')); };
(async () => {
  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 900, height: 1000 } });
  const errors = []; page.on('pageerror', e => errors.push(String(e)));
  // stand-ins for the two pickers: a folder and a file kept in memory, their writes hashed with the page's own SHA-1
  await page.addInitScript(() => {
    window.__fake = { files: {}, pre: [], dirCalls: 0, saveCalls: 0, cancel: false };
    const fileHandle = name => ({ name, createWritable: async () => ({ write: async b => { window.__fake.files[name] = new Uint8Array(b); }, close: async () => {} }) });
    window.showDirectoryPicker = async () => { window.__fake.dirCalls++; if (window.__fake.cancel) { const e = new Error('cancel'); e.name = 'AbortError'; throw e; }
      return { name: 'My Build', getFileHandle: async (name, o) => { if (!(o && o.create) && !window.__fake.pre.includes(name) && !(name in window.__fake.files)) { const e = new Error('nf'); e.name = 'NotFoundError'; throw e; } return fileHandle(name); } }; };
    window.showSaveFilePicker = async () => { window.__fake.saveCalls++; return fileHandle('default.xbe'); };
  });
  await page.goto(PAGE);
  const txt = id => page.locator('#' + id).innerText();
  const dis = id => page.locator('#' + id).isDisabled();
  const hashOf = name => page.evaluate(n => window.__fake.files[n] ? sgHexOf(sgSha1(window.__fake.files[n])) : null, name);
  say('the page shows v2.0', (await page.locator('.sub').innerText()).includes('v2.0'));
  say('at first: 360 not ticked, the xefu1_1.xex button hidden, every build off', !(await page.locator('#x360').isChecked()) && !(await page.locator('#xexlabel').isVisible()) && await dis('b10') && await dis('b10tu') && await dis('b15'));
  await page.setInputFiles('#xbe', X10);
  await page.waitForFunction(() => document.getElementById('detect').className.includes('ok'));
  say('the v1.0 default.xbe is recognised; v1.0 and 1.0 TU Skip on, v1.5 off', (await txt('detect')).includes('v1.0') && !(await dis('b10')) && !(await dis('b10tu')) && await dis('b15'));
  await page.screenshot({ path: require('os').tmpdir() + '/ui_1_plain.png' });
  // without 360: the Save dialog, one file
  await page.click('#b10tu'); await page.waitForFunction(() => document.getElementById('status').className.includes('ok'));
  say('360 not ticked: the Save dialog, default.xbe only, identical to the 1.0 TU Skip build', (await hashOf('default.xbe')) === want.tu && (await hashOf('xefu1_1.xex')) === null && await page.evaluate(() => window.__fake.saveCalls === 1 && window.__fake.dirCalls === 0), await txt('status'));
  // tick 360
  await page.evaluate(() => { window.__fake.files = {}; });
  await page.check('#x360');
  say('360 ticked: the xefu1_1.xex button appears beside the other; builds off until it is chosen; Step 2 says a folder', await page.locator('#xexlabel').isVisible() && await dis('b10') && await dis('b10tu') && (await txt('step2')).includes('folder'));
  const a = await page.locator('label[for=xbe]').boundingBox(), b = await page.locator('#xexlabel').boundingBox();
  say('... to the right of the default.xbe button, on the same line, the same height', b.x > a.x + a.width - 1 && Math.abs(b.y - a.y) < 2 && Math.abs(b.height - a.height) < 1, JSON.stringify([a, b]));
  await page.setInputFiles('#xex', E64);
  await page.waitForFunction(() => document.getElementById('detect360').className.includes('ok') || document.getElementById('detect360').className.includes('bad'), null, { timeout: 60000 });
  say('the original xefu1_1.xex is recognised; the builds come on', (await txt('detect360')).includes('the original') && (await txt('detect360')).includes('R89') && !(await dis('b10')) && !(await dis('b10tu')) && await dis('b15'), await txt('detect360'));
  await page.screenshot({ path: require('os').tmpdir() + '/ui_2_360.png' });
  await page.click('#b10tu'); await page.waitForFunction(() => /ok|bad/.test(document.getElementById('status').className), null, { timeout: 60000 });
  say('360 ticked: ONE folder asked for; default.xbe and xefu1_1.xex both written there, identical to the builds', (await hashOf('default.xbe')) === want.tu && (await hashOf('xefu1_1.xex')) === want.emu && await page.evaluate(() => window.__fake.dirCalls === 1 && window.__fake.saveCalls === 1), await txt('status'));
  say('... and the page says so', (await txt('status')).includes('default.xbe') && (await txt('status')).includes('xefu1_1.xex') && (await txt('status')).includes('My Build'), await txt('status'));
  await page.screenshot({ path: require('os').tmpdir() + '/ui_3_saved.png' });
  // a folder that already has the files: asked first
  let asked = 0, answer = false;
  page.on('dialog', async d => { asked++; if (answer) await d.accept(); else await d.dismiss(); });
  await page.evaluate(() => { window.__fake.files = {}; window.__fake.pre = ['default.xbe']; });
  await page.click('#b10'); await page.waitForFunction(() => document.getElementById('status').textContent.includes('Nothing saved'), null, { timeout: 60000 });
  say('a folder that already has default.xbe: asked first; Cancel writes nothing', asked === 1 && await page.evaluate(() => Object.keys(window.__fake.files).length === 0), await txt('status'));
  answer = true;
  await page.click('#b10'); await page.waitForFunction(() => /ok|bad/.test(document.getElementById('status').className), null, { timeout: 60000 });
  say('... OK replaces: both written, identical to the v1.0 build and the emulator', asked === 2 && (await hashOf('default.xbe')) === want.v10 && (await hashOf('xefu1_1.xex')) === want.emu, await txt('status'));
  // the folder dialog cancelled
  await page.evaluate(() => { window.__fake.files = {}; window.__fake.pre = []; window.__fake.cancel = true; });
  await page.click('#b10'); await page.waitForFunction(() => document.getElementById('status').textContent.includes('No folder chosen'));
  say('the folder dialog cancelled: nothing written', await page.evaluate(() => Object.keys(window.__fake.files).length === 0));
  await page.evaluate(() => { window.__fake.cancel = false; });
  // the other original, and the v1.5 XBE
  await page.setInputFiles('#xex', E128);
  await page.waitForFunction(() => document.getElementById('detect360').textContent.includes('128 MB'), null, { timeout: 60000 });
  await page.setInputFiles('#xbe', X15);
  await page.waitForFunction(() => document.getElementById('detect').textContent.includes('v1.5'));
  say('the original with the 128 MB change is recognised too; with a v1.5 XBE only v1.5 is on', await dis('b10') && await dis('b10tu') && !(await dis('b15')), await txt('detect360'));
  await page.click('#b15'); await page.waitForFunction(() => /ok|bad/.test(document.getElementById('status').className), null, { timeout: 60000 });
  say('... v1.5 and the emulator written, identical to the builds', (await hashOf('default.xbe')) === want.v15 && (await hashOf('xefu1_1.xex')) === want.emu, await txt('status'));
  // a wrong xefu1_1.xex
  await page.setInputFiles('#xex', R89);
  await page.waitForFunction(() => document.getElementById('detect360').className.includes('bad'), null, { timeout: 60000 });
  say('the release itself given as the original: refused, and the builds go off', (await txt('detect360')).includes('already') && await dis('b15'), await txt('detect360'));
  await page.screenshot({ path: require('os').tmpdir() + '/ui_4_refused.png' });
  // unticking 360 brings the builds back and hides the second button
  await page.uncheck('#x360');
  say('360 unticked again: the second button and its lines hidden; the build is on from the default.xbe alone', !(await page.locator('#xexlabel').isVisible()) && !(await page.locator('#detect360').isVisible()) && !(await dis('b15')) && (await txt('step2')).includes('where to save'));
  // a browser with neither picker: two downloads
  const p2 = await browser.newPage({ acceptDownloads: true });
  await p2.addInitScript(() => { delete window.showDirectoryPicker; delete window.showSaveFilePicker; window.showDirectoryPicker = undefined; window.showSaveFilePicker = undefined; });
  await p2.goto(PAGE);
  await p2.setInputFiles('#xbe', X10); await p2.check('#x360'); await p2.setInputFiles('#xex', E64);
  await p2.waitForFunction(() => document.getElementById('detect360').className.includes('ok'), null, { timeout: 60000 });
  const got = []; p2.on('download', d => got.push(d));
  await p2.click('#b10tu'); await p2.waitForFunction(() => /ok|bad/.test(document.getElementById('status').className), null, { timeout: 60000 });
  await p2.waitForTimeout(1500);
  const names = got.map(d => d.suggestedFilename()).sort(); const hs = {};
  for (const d of got) hs[d.suggestedFilename()] = sha(await d.path());
  say('a browser with no folder dialog: both files downloaded, identical to the builds', names.join() === 'default.xbe,xefu1_1.xex' && hs['default.xbe'] === want.tu && hs['xefu1_1.xex'] === want.emu, names.join());
  // narrow screen: the buttons still fit
  const p3 = await browser.newPage({ viewport: { width: 380, height: 800 } });
  await p3.goto(PAGE); await p3.check('#x360'); await p3.screenshot({ path: require('os').tmpdir() + '/ui_5_narrow.png' });
  const sw = await p3.evaluate(() => document.documentElement.scrollWidth); say('at 380 px wide the page does not scroll sideways', sw <= 380, String(sw));
  say('no script errors on the page', errors.length === 0, errors.join(' | '));
  await browser.close();
  console.log(ok ? 'PAGE CHECKS PASSED' : 'PAGE CHECKS FAILED'); process.exit(ok ? 0 : 1);
})();
