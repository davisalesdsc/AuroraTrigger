// node render_png.js <svgDir> <pngDir>  — renders every SVG to a transparent PNG at 3x
const { chromium } = require('playwright');
const fs = require('fs'), path = require('path');
(async () => {
  const [svgDir, pngDir] = process.argv.slice(2);
  fs.mkdirSync(pngDir, { recursive: true });
  const browser = await chromium.launch();
  const ctx = await browser.newContext({ deviceScaleFactor: 3 });
  const page = await ctx.newPage();
  for (const f of fs.readdirSync(svgDir).filter(f => f.endsWith('.svg'))) {
    const svg = fs.readFileSync(path.join(svgDir, f), 'utf8');
    const m = svg.match(/width="([\d.]+)" height="([\d.]+)"/);
    await page.setViewportSize({ width: Math.ceil(+m[1]), height: Math.ceil(+m[2]) });
    await page.setContent(`<html><body style="margin:0;background:transparent">${svg}</body></html>`);
    await page.screenshot({ path: path.join(pngDir, f.replace('.svg', '.png')), omitBackground: true,
                            clip: { x: 0, y: 0, width: +m[1], height: +m[2] } });
  }
  await browser.close();
})();
