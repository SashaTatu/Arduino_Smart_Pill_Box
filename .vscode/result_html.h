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
    --color-water-deep: #03a9f4;
    --color-pill-white: #ffffff;
    --color-text: #333;
  }

  body { 
    font-family: 'Segoe UI', Roboto, sans-serif; 
    display: flex; 
    justify-content: center; 
    align-items: center; 
    height: 100vh; 
    margin: 0; 
    background: linear-gradient(135deg, #e0f2f1 0%, #b2ebf2 100%);
    color: var(--color-text);
  }

  .card { 
    background: var(--color-pill-white); 
    padding: 40px; 
    border-radius: 28px; 
    width: 90%; 
    max-width: 360px; 
    text-align: center; 
    box-shadow: 0 15px 35px rgba(0,0,0,0.1), inset 0 0 0 2px var(--color-glass-rim);
    position: relative;
    overflow: hidden;
    animation: fadeIn 0.5s ease-out;
  }

  .card::after {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    height: 8px;
    background-color: var(--color-water-deep);
  }

  h2 { 
    color: var(--color-water-deep); 
    margin-top: 0;
    font-size: 1.6em;
  }

  p { font-size: 15px; color: #666; margin-bottom: 20px; }

  #deviceIdBox { 
    background: #f0faff; 
    color: var(--color-water-deep); 
    padding: 16px; 
    border-radius: 15px; 
    font-family: 'Courier New', monospace; 
    font-weight: bold;
    margin: 20px 0; 
    word-break: break-all; 
    font-size: 18px; 
    border: 1px dashed var(--color-water-deep);
  }

  button { 
    background: var(--color-water-deep); 
    color: #fff; 
    border: none; 
    border-radius: 50px; 
    padding: 14px 25px; 
    cursor: pointer; 
    font-size: 15px; 
    font-weight: 600;
    width: 100%;
    transition: all 0.2s;
    box-shadow: 0 4px 12px rgba(3, 169, 244, 0.2);
    text-transform: uppercase;
  }

  button:hover { 
    background: #0288d1; 
    transform: translateY(-1px);
  }

  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
  }
</style>
<script>
function copyId() {
  const id = document.getElementById("deviceIdBox").innerText;
  navigator.clipboard.writeText(id).then(() => {
    const btn = document.querySelector('button');
    const originalText = btn.innerText;
    btn.innerText = "СКОПІЙОВАНО!";
    btn.style.background = "#4caf50";
    setTimeout(() => {
      btn.innerText = originalText;
      btn.style.background = "";
    }, 2000);
  });
}
</script>
</head>
<body>
<div class="card">
  <h2>Готово!</h2>
  <p>Пристрій успішно налаштовано. Збережіть цей ID для додавання в додаток:</p>
  <div id="deviceIdBox">%DEVICE_ID%</div>
  <button onclick="copyId()">Скопіювати ID</button>
</div>
</body>
</html>
)rawliteral";

    html.replace("%DEVICE_ID%", deviceId);
    return html;
}