#pragma once

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="UTF-8">
<title>SafePoint — Wi-Fi Налаштування</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  :root {
    --color-water-deep: #0288d1;
    --color-water-light: #03a9f4;
    --color-pill-white: #ffffff;
    --color-text: #1a1a1a;
    --color-text-light: #666;
    --color-placeholder: #999;
  }

  * {
    box-sizing: border-box;
  }

  body {
    font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif;
    background: linear-gradient(135deg, #e0f2f1 0%, #b2ebf2 50%, #80deea 100%);
    color: var(--color-text);
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
    margin: 0;
    padding: 20px;
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
    padding: 50px 40px;
    border-radius: 32px;
    width: 100%;
    max-width: 440px;
    text-align: center;
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.15),
                0 0 1px rgba(255, 255, 255, 0.6) inset;
    border: 1px solid rgba(255, 255, 255, 0.8);
    position: relative;
    overflow: hidden;
    animation: slideUp 0.6s cubic-bezier(0.34, 1.56, 0.64, 1);
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

  .card::after {
    content: '';
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    height: 4px;
    background: linear-gradient(90deg, var(--color-water-deep), var(--color-water-light));
  }

  h2 {
    margin: 0 0 30px 0;
    font-size: 2.2em;
    font-weight: 700;
    background: linear-gradient(135deg, var(--color-water-deep) 0%, var(--color-water-light) 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    letter-spacing: -1px;
    animation: fadeInText 0.8s ease-out;
  }

  .input-group {
    margin-bottom: 16px;
    text-align: left;
    animation: fadeInText 0.8s ease-out;
  }

  .input-group label {
    display: block;
    font-size: 12px;
    font-weight: 600;
    color: var(--color-text-light);
    margin-bottom: 6px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  input {
    width: 100%;
    padding: 14px 18px;
    border: 2px solid #e0e0e0;
    border-radius: 12px;
    background: linear-gradient(135deg, #fafafa 0%, #f5f5f5 100%);
    font-size: 15px;
    color: var(--color-text);
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    letter-spacing: 0.3px;
  }

  input:focus {
    outline: none;
    border-color: var(--color-water-deep);
    background: #fff;
    box-shadow: 0 0 0 4px rgba(2, 136, 209, 0.1),
                0 8px 20px rgba(2, 136, 209, 0.15);
    transform: translateY(-2px);
  }

  input::placeholder {
    color: var(--color-placeholder);
  }

  button {
    background: linear-gradient(135deg, var(--color-water-deep) 0%, var(--color-water-light) 100%);
    color: #fff;
    border: none;
    border-radius: 12px;
    padding: 16px 24px;
    cursor: pointer;
    font-size: 15px;
    font-weight: 700;
    width: 100%;
    margin-top: 24px;
    transition: all 0.3s cubic-bezier(0.34, 1.56, 0.64, 1);
    box-shadow: 0 6px 20px rgba(2, 136, 209, 0.35);
    text-transform: uppercase;
    letter-spacing: 0.5px;
    position: relative;
    overflow: hidden;
    animation: fadeInText 0.8s ease-out 0.1s backwards;
  }

  button::before {
    content: '';
    position: absolute;
    top: 50%;
    left: 50%;
    width: 0;
    height: 0;
    border-radius: 50%;
    background: rgba(255, 255, 255, 0.3);
    transform: translate(-50%, -50%);
    transition: width 0.6s, height 0.6s;
  }

  button:active::before {
    width: 300px;
    height: 300px;
  }

  button:hover {
    background: linear-gradient(135deg, var(--color-water-light) 0%, var(--color-water-deep) 100%);
    transform: translateY(-2px);
    box-shadow: 0 10px 30px rgba(2, 136, 209, 0.45);
  }

  button:active {
    transform: translateY(0);
    box-shadow: 0 3px 10px rgba(2, 136, 209, 0.3);
  }

  .footer-text {
    font-size: 13px;
    color: var(--color-text-light);
    margin-top: 28px;
    line-height: 1.6;
    padding: 16px;
    background: rgba(2, 136, 209, 0.05);
    border-radius: 12px;
    animation: fadeInText 0.8s ease-out 0.2s backwards;
  }

  @keyframes slideUp {
    from {
      opacity: 0;
      transform: translateY(30px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }

  @keyframes fadeInText {
    from {
      opacity: 0;
      transform: translateY(10px);
    }
    to {
      opacity: 1;
      transform: translateY(0);
    }
  }

  @keyframes shimmer {
    0%, 100% { opacity: 0; }
    50% { opacity: 1; }
  }

  @media (max-width: 480px) {
    .card {
      padding: 40px 30px;
      border-radius: 24px;
    }

    h2 {
      font-size: 1.9em;
      margin-bottom: 25px;
    }

    input {
      padding: 12px 16px;
      font-size: 14px;
    }

    button {
      padding: 14px 20px;
      font-size: 14px;
      margin-top: 20px;
    }
  }
</style>
</head>
<body>
<div class="card">
  <h2>SafePoint</h2>
  <form action="/connect" method="post">
    <div class="input-group">
      <label for="ssid">Мережа</label>
      <input type="text" id="ssid" name="ssid" placeholder="Введіть назву мережі (SSID)" required>
    </div>
    <div class="input-group">
      <label for="password">Пароль</label>
      <input type="password" id="password" name="password" placeholder="Введіть пароль Wi-Fi">
    </div>
    <button type="submit">🔗 Підключити</button>
  </form>
  <p class="footer-text">💡 Введіть дані вашої Wi-Fi мережи для активації пристрою SafePoint</p>
</div>
</body>
</html>
)rawliteral";