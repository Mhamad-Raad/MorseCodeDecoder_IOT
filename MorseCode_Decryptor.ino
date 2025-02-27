#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "iQ Dlsoz 2.4";
const char* password = "07725202123";

const int buttonPin = D1;
LiquidCrystal_I2C lcd(0x27, 16, 2);
ESP8266WebServer server(80);

const int dotDuration = 250;  // Hold for less than 250ms for a dot
const int dashDuration = 500; // Hold for more than 500ms for a dash
const int letterGap = 1000;   // Wait 1 second for next letter
const int wordGap = 5000;     // Wait 2 seconds for space
const int deleteDuration = 3000;  // Hold for 3 seconds to delete a letter


unsigned long buttonPressStart = 0;
unsigned long lastButtonRelease = 0;
String currentMorseSequence = "";
String currentEnglishText = "";
String webMorseOutput = "";

const String MORSE_TO_CHAR[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",  // A-J
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",    // K-T
  "..-", "...-", ".--", "-..-", "-.--", "--..",                            // U-Z
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----." // 0-9
};

char morseToChar(String morse) {
  for (int i = 0; i < 26; i++) {
    if (MORSE_TO_CHAR[i] == morse) return char('A' + i);  // Letters A-Z
  }
  for (int i = 0; i < 10; i++) {
    if (MORSE_TO_CHAR[26 + i] == morse) return char('0' + i);  // Numbers 0-9
  }
  return '\0'; // Return null character if not found
}

String charToMorse(char c) {
  if (c >= 'A' && c <= 'Z') return MORSE_TO_CHAR[c - 'A'];  // A-Z
  if (c >= 'a' && c <= 'z') return MORSE_TO_CHAR[c - 'a'];  // a-z (converted to uppercase)
  if (c >= '0' && c <= '9') return MORSE_TO_CHAR[26 + (c - '0')];  // 0-9
  if (c == ' ') return "/";  // Space
  return "";
}

void handleRoot() {
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <style>
        body {
          font-family: Arial, sans-serif;
          margin: 0;
          padding: 20px;
          background: #f0f2f5;
        }
        .container {
          max-width: 600px;
          margin: 0 auto;
          background: white;
          padding: 20px;
          border-radius: 10px;
          box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1 {
          color: #1a73e8;
          text-align: center;
          margin-bottom: 30px;
        }
        .input-group {
          margin-bottom: 20px;
        }
        input[type='text'] {
          width: 100%;
          padding: 12px;
          border: 2px solid #ddd;
          border-radius: 6px;
          font-size: 16px;
          box-sizing: border-box;
          margin-bottom: 10px;
        }
        .output {
          background: #f8f9fa;
          padding: 15px;
          border-radius: 6px;
          min-height: 50px;
          word-wrap: break-word;
          margin-bottom: 15px;
        }
        .button-group {
          display: flex;
          gap: 10px;
        }
        button {
          flex: 1;
          padding: 12px;
          color: white;
          border: none;
          border-radius: 6px;
          font-size: 16px;
          cursor: pointer;
          margin-top: 10px;
        }
        .submit-btn {
          background: #1a73e8;
        }
        .submit-btn:hover {
          background: #1557b0;
        }
        .reset-btn {
          background: #dc3545;
        }
        .reset-btn:hover {
          background: #bb2d3b;
        }
        .label {
          font-weight: bold;
          margin-bottom: 8px;
          color: #444;
        }
      </style>
    </head>
    <body>
      <div class='container'>
        <h1>Morse Code Translator</h1>
        <div class='input-group'>
          <div class='label'>Enter Text:</div>
          <input type='text' id='textInput'>
          <div class='button-group'>
            <button class='submit-btn' onclick='submitText()'>Submit</button>
            <button class='reset-btn' onclick='resetDisplay()'>Reset</button>
          </div>
        </div>
        <div class='input-group'>
          <div class='label'>Morse Code:</div>
          <div class='output' id='morseOutput'></div>
        </div>
      </div>
      <script>
        function submitText() {
          let text = document.getElementById('textInput').value;
          fetch('/translate?text=' + encodeURIComponent(text))
            .then(response => response.text())
            .then(morse => {
              document.getElementById('morseOutput').innerText = morse;
              console.log('Translation received:', morse);
            })
            .catch(error => {
              console.error('Error:', error);
              alert('Translation failed. Please try again.');
            });
        }

        function resetDisplay() {
          fetch('/translate?text=')
          
          fetch('/reset')
            .then(response => {
              if(response.ok) {
                document.getElementById('textInput').value = '';
                document.getElementById('morseOutput').innerText = '';
                console.log('Display reset');
              }
            })
            .catch(error => {
              console.error('Reset failed:', error);
              alert('Reset failed. Please try again.');
            });
        }
      </script>
    </body>
    </html>
  )";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Morse Code Translator...");
  
  Wire.begin(D2, D3);
  Serial.println("Initializing I2C LCD...");
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Morse Translator");
  delay(1000);
  
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.println("Button configured on D1");
  
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  server.on("/", handleRoot);
  server.on("/translate", handleTranslate);
  server.on("/reset", handleReset);
  server.begin();
  
  Serial.println("\nSystem Ready!");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  
  lcd.clear();
  lcd.print("IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(7000);
  lcd.clear();
}

void handleReset() {
  currentEnglishText = "";
  currentMorseSequence = "";
  Serial.println("Reset from web interface");
  server.send(200, "text/plain", "Reset successful");
}

void loop() {
  server.handleClient();
  handleMorseInput();
  updateDisplay();
  delay(10);
}

void handleMorseInput() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(buttonPin);
  static bool letterProcessed = false;
  unsigned long currentTime = millis();
  
  // Button press started
  if (buttonState == LOW && lastButtonState == HIGH) {
    buttonPressStart = currentTime;
    letterProcessed = false;
    Serial.println("Button pressed");
  }
  
  // Button released
  if (buttonState == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = currentTime - buttonPressStart;
    if (pressDuration > 50) {  // Debounce
      if (pressDuration >= deleteDuration) {
        // DELETE LAST CHARACTER if held for 3+ seconds
        if (currentEnglishText.length() > 0) {
          currentEnglishText.remove(currentEnglishText.length() - 1);
          Serial.println("Deleted last character");
        }
      } else if (pressDuration < dashDuration) {
        currentMorseSequence += ".";
        Serial.println("DOT added");
      } else {
        currentMorseSequence += "-";
        Serial.println("DASH added");
      }
      Serial.println("Current sequence: " + currentMorseSequence);
    }
    lastButtonRelease = currentTime;
  }

  // Letter gap detection (1 sec)
  if (buttonState == HIGH && !letterProcessed && 
      (currentTime - lastButtonRelease) > letterGap && 
      currentMorseSequence.length() > 0) {
    char decodedChar = morseToChar(currentMorseSequence);
    if (decodedChar != '\0') {
      currentEnglishText += decodedChar;
      Serial.println("Decoded letter: " + String(decodedChar));
    }
    currentMorseSequence = "";
    letterProcessed = true;
  }

  // Word gap detection (7 sec for space)
  if (buttonState == HIGH && 
      (currentTime - lastButtonRelease) > wordGap && 
      currentEnglishText.length() > 0 && 
      currentEnglishText[currentEnglishText.length() - 1] != ' ') {
    currentEnglishText += " ";
    Serial.println("Space added");
  }

  lastButtonState = buttonState;
}

void updateDisplay() {
  static unsigned long lastScrollTime = 0;
  static int scrollIndex1 = 0;
  static int scrollIndex2 = 0;
  const int scrollDelay = 700;  // Increased delay for slower scrolling

  String line1 = currentEnglishText;
  String line2 = webMorseOutput;

  // If text is longer than 16 chars, enable scrolling
  if (line1.length() > 16) {
    if (millis() - lastScrollTime > scrollDelay) {
      scrollIndex1 = (scrollIndex1 + 1) % (line1.length() + 1);
      lastScrollTime = millis();
    }
    line1 = line1.substring(scrollIndex1, scrollIndex1 + 16);
  }

  if (line2.length() > 16) {
    if (millis() - lastScrollTime > scrollDelay) {
      scrollIndex2 = (scrollIndex2 + 1) % (line2.length() + 1);
      lastScrollTime = millis();
    }
    line2 = line2.substring(scrollIndex2, scrollIndex2 + 16);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);

  Serial.println("\nLCD Update:");
  Serial.println("Line 1: " + line1);
  Serial.println("Line 2: " + line2);
}



void handleTranslate() {
  if (server.hasArg("text")) {
    String input = server.arg("text");
    Serial.println("\nWeb Translation Request: " + input);
    
    String morse = "";
    for (unsigned int i = 0; i < input.length(); i++) {
      String code = charToMorse(input[i]);
      if (code != "") {
        morse += code + " ";
      }
    }
    webMorseOutput = morse;
    Serial.println("Translated to: " + morse);
    server.send(200, "text/plain", morse);
  }
}
