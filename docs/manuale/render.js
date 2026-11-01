const { chromium } = require('playwright');
const path = require('path');
(async () => {
  const out = process.argv[2];
  const b = await chromium.launch();
  const url = 'file://' + path.resolve(__dirname, 'src/index.html');
  // PDF A4
  let p = await b.newPage();
  await p.goto(url, { waitUntil: 'networkidle' });
  await p.evaluate(() => document.fonts.ready);
  await p.pdf({ path: path.join(out, 'Phonogene_manuale.pdf'), preferCSSPageSize: true, printBackground: true });
  // schermo: desktop e mobile
  for (const [w, name] of [[1280, 'screen_desktop.png'], [390, 'screen_mobile.png']]) {
    p = await b.newPage({ viewport: { width: w, height: 900 } });
    await p.goto(url, { waitUntil: 'networkidle' });
    await p.evaluate(() => document.fonts.ready);
    await p.screenshot({ path: path.join(out, name), fullPage: true });
  }
  await b.close();
})();
