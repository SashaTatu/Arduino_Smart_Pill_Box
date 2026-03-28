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
    /* Кольори, взяті безпосередньо з логотипу */
    --color-bg: #e0f2f1;           /* Дуже світлий фон сторінки */
    --color-glass-rim: #ced4da;    /* Світло-сірий метал контуру */
    --color-water-shallow: #b3e5fc;/* Світло-блакитний верх скла */
    --color-water-deep: #03a9f4;   /* Насичений синій води */
    --color-text: #333;            /* Темний текст для контрасту на білому */
    --color-pill-white: #ffffff;   /* Білий колір таблетки та фону картки */
  }

  body {
    font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
    background-color: var(--color-bg);
    color: var(--color-text);
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100vh;
    margin: 0;
    /* Легкий градієнт на фоні для глибини */
    background-image: linear-gradient(135deg, #e0f2f1 0%, #b2ebf2 100%);
  }

  .card {
    background-color: var(--color-pill-white);
    padding: 40px;
    /* Закруглення, як у самого значка */
    border-radius: 28px; 
    width: 90%;
    max-width: 380px;
    text-align: center;
    /* Ефект об'ємного емалевого значка */
    box-shadow: 
      0 15px 35px rgba(0,0,0,0.1), /* М'яка тінь знизу */
      inset 0 0 0 2px var(--color-glass-rim); /* Внутрішній металевий контур */
    position: relative;
    overflow: hidden;
  }

  /* Декоративний елемент: синя смуга знизу, як вода у склі */
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
    margin-top: 0;
    margin-bottom: 25px;
    font-size: 1.8em;
    font-weight: 600;
    color: var(--color-water-deep); /* Заголовок синім кольором води */
    letter-spacing: -0.5px;
  }

  .input-group {
    margin-bottom: 18px;
    text-align: left;
  }

  input {
    width: 100%;
    padding: 14px 18px;
    border: 2px solid var(--color-glass-rim); /* Металевий контур, як на лого */
    /* Супер-закруглення, як у таблетки */
    border-radius: 50px; 
    background-color: #fbfbfb;
    font-size: 16px;
    color: var(--color-text);
    box-sizing: border-box;
    transition: all 0.2s ease;
  }

  input:focus {
    outline: none;
    border-color: var(--color-water-deep);
    background-color: #fff;
    box-shadow: 0 0 0 4px var(--color-water-shallow);
  }

  input::placeholder {
    color: #a0a0a0;
  }

  button {
    background-color: var(--color-water-deep); /* Кнопка насиченого синього кольору */
    color: #fff;
    border: none;
    /* Супер-закруглення, як у таблетки */
    border-radius: 50px; 
    padding: 15px;
    cursor: pointer;
    font-size: 17px;
    font-weight: 600;
    width: 100%;
    margin-top: 15px;
    transition: all 0.2s ease;
    box-shadow: 0 4px 12px rgba(3, 169, 244, 0.3);
    text-transform: uppercase;
    letter-spacing: 1px;
  }

  button:hover {
    background-color: #0288d1; /* Трохи темніший синій при наведенні */
    transform: translateY(-1px);
    box-shadow: 0 6px 15px rgba(3, 169, 244, 0.4);
  }
  
  button:active {
    transform: translateY(1px);
    box-shadow: 0 2px 6px rgba(3, 169, 244, 0.3);
  }

  .footer-text {
    font-size: 14px;
    color: #757575;
    margin-top: 25px;
    line-height: 1.5;
    padding: 0 10px;
  }

  /* Анімація появи */
  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(20px); }
    to { opacity: 1; transform: translateY(0); }
  }

  .card {
    animation: fadeIn 0.5s ease-out;
  }
</style>
</head>
<body>
<div class="card">
  <h2>SafePoint</h2>
  <form action="/connect" method="post">
    <div class="input-group">
      <input type="text" name="ssid" placeholder="Назва мережі (SSID)" required>
    </div>
    <div class="input-group">
      <input type="password" name="password" placeholder="Пароль Wi-Fi">
    </div>
    <button type="submit">Підключити</button>
  </form>
  <p class="footer-text">Введіть дані вашої Wi-Fi мережі для активації пристрою</p>
</div>
</body>
</html>
)rawliteral";