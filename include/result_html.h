String getResultPage(const String& deviceId, const String& apiResult) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<title>SafePoint — Успішно</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  :root {
    --color-bg: #e0f2f1;
    --color-glass-rim: #ced4da;
    --color-water-shallow: #b3e5fc;
    --color-water-deep: #0288d1;
    --color-pill-white: #ffffff;
    --color-success: #4caf50;
    --color-success-light: #66bb6a;
    --color-text: #1a1a1a;
    --color-text-light: #666;
  }

  * { 
    box-sizing: border-box; 
  }

  body { 
    font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif; 
    display: flex; 
    justify-content: center; 
    align-items: center; 
    min-height: 100vh; 
    margin: 0; 
    padding: 20px;
    background: linear-gradient(135deg, #e0f2f1 0%, #b2ebf2 50%, #80deea 100%);
    color: var(--color-text);
    animation: gradientShift 15s ease infinite;
    background-size: 200% 200%;
  }

  @keyframes gradientShift {
    0%, 100% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
  }

  .card { 
    background: rgba(255, 255, 255, 0.95);
    backdrop-filter: blur(10px);
    padding: 45px 35px;
    border-radius: 32px; 
    width: 100%;
    max-width: 420px; 
    text-align: center;
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.15), 
                0 0 1px rgba(255, 255, 255, 0.6) inset;
    position: relative;
    overflow: hidden;
    animation: slideUp 0.6s cubic-bezier(0.34, 1.56, 0.64, 1);
    border: 1px solid rgba(255, 255, 255, 0.8);
  }

  .card::before {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    right: 0;
    height: 3px;
    background: linear-gradient(90deg, transparent, var(--color-water-deep), transparent);
    animation: shimmer 3s ease-in-out infinite;
  }

  .success-icon {
    width: 80px;
    height: 80px;
    margin: 0 auto 20px;
    background: linear-gradient(135deg, var(--color-success) 0%, var(--color-success-light) 100%);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 40px;
    animation: popIn 0.6s cubic-bezier(0.34, 1.56, 0.64, 1);
    box-shadow: 0 10px 30px rgba(76, 175, 80, 0.3);
  }

  h2 { 
    color: var(--color-success); 
    margin: 15px 0 5px 0;
    font-size: 1.9em;
    font-weight: 700;
    letter-spacing: -0.5px;
    animation: fadeInText 0.8s ease-out 0.2s backwards;
  }

  .subtitle {
    color: var(--color-text-light);
    font-size: 14px;
    margin-top: 5px;
    font-weight: 500;
    animation: fadeInText 0.8s ease-out 0.3s backwards;
  }

  p { 
    font-size: 14px; 
    color: var(--color-text-light); 
    margin: 20px 0 15px 0; 
    line-height: 1.6;
    animation: fadeInText 0.8s ease-out 0.4s backwards;
  }

  #deviceIdBox { 
    background: linear-gradient(135deg, #f0faff 0%, #e1f5fe 100%); 
    color: var(--color-water-deep); 
    padding: 16px 18px;
    border-radius: 14px; 
    font-family: 'Courier New', monospace; 
    font-weight: 700;
    margin: 20px 0; 
    word-break: break-all; 
    font-size: 15px; 
    border: 2px dashed var(--color-water-deep);
    letter-spacing: 0.5px;
    animation: fadeInText 0.8s ease-out 0.5s backwards;
    box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.05);
  }

  .controls {
    margin-top: 25px;
    animation: fadeInText 0.8s ease-out 0.6s backwards;
  }

  .btn { 
    border: none; 
    border-radius: 12px; 
    padding: 14px 24px; 
    cursor: pointer; 
    font-size: 13px; 
    font-weight: 700;
    width: 100%;
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    text-transform: uppercase;
    margin-bottom: 12px;
    display: block;
    text-decoration: none;
    letter-spacing: 0.5px;
    position: relative;
    overflow: hidden;
  }

  .btn-copy {
    background: linear-gradient(135deg, #f5f5f5 0%, #e8e8e8 100%);
    color: var(--color-text);
    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.08);
  }

  .btn-copy:hover { 
    background: linear-gradient(135deg, #e8e8e8 0%, #d8d8d8 100%);
    transform: translateY(-2px);
  }

  .btn-start {
    background: linear-gradient(135deg, var(--color-success) 0%, var(--color-success-light) 100%);
    color: #fff;
    box-shadow: 0 6px 20px rgba(76, 175, 80, 0.35);
    margin-top: 15px;
  }

  .btn-start:hover { 
    transform: translateY(-2px);
    box-shadow: 0 10px 30px rgba(76, 175, 80, 0.45);
  }

  .footer-text {
    font-size: 11px; 
    margin-top: 20px; 
    color: #aaa;
    animation: fadeInText 0.8s ease-out 0.7s backwards;
  }

  @keyframes slideUp { from { opacity: 0; transform: translateY(30px); } to { opacity: 1; transform: translateY(0); } }
  @keyframes popIn { 0% { opacity: 0; transform: scale(0.3); } 100% { opacity: 1; transform: scale(1); } }
  @keyframes fadeInText { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
  @keyframes shimmer { 0%, 100% { opacity: 0.2; } 50% { opacity: 1; } }

  @media (max-width: 480px) {
    .card { padding: 35px 25px; }
    h2 { font-size: 1.6em; }
  }
</style>
<script>
function copyId() {
  const id = document.getElementById("deviceIdBox").innerText;
  navigator.clipboard.writeText(id).then(() => {
    const btn = document.getElementById('copyBtn');
    const originalText = btn.innerText;
    btn.innerText = "✓ СКОПІЙОВАНО!";
    btn.style.background = "#e8f5e9";
    btn.style.color = "#2e7d32";
    setTimeout(() => {
      btn.innerText = originalText;
      btn.style.background = "";
      btn.style.color = "";
    }, 2500);
  });
}
</script>
</head>
<body>
<div class="card">
  <div class="success-icon">✔</div>
  
  <h2>Чудово!</h2>
  <p class="subtitle">Пристрій успішно налаштовано</p>
  
  <p>Скопіюйте цей ID для реєстрації в Telegram-боті:</p>
  
  <div id="deviceIdBox">%DEVICE_ID%</div>
  
  <div class="controls">
    <button id="copyBtn" class="btn btn-copy" onclick="copyId()">📋 Скопіювати ID</button>

    <form action="/reboot" method="POST" style="margin: 0; width: 100%;" onsubmit="setTimeout(function(){ window.close(); }, 500);">
      <button type="submit" class="btn btn-start">🚀 Запустити пристрій</button>
    </form>
  </div>
  
  <p class="footer-text">💡 Після натискання ESP32 перезавантажиться і перейде в робочий режим</p>
</div>
</body>
</html>
)rawliteral";

    html.replace("%DEVICE_ID%", deviceId);
    return html;
}