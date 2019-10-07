#include <Arduino.h>
// usable pins D0,D1,D2,D5,D6,D7 (D10 is TX (GPIO1), D9 is RX (GPIO3), D3 is GPIO0, D4 is GPIO2, D8 is GPIO15)
const int     FWD_PIN       = D0;
const int     BWD_PIN       = D1;
const int     OPT_PIN       = D2;

const int     BUT_PIN       = D5;

const size_t  CUT           = 0x00;
const size_t  THROUGH       = 0x01;

/* To know if axternal */
boolean machineStarted();
void startMachine();
void stopMachine();
boolean becameStuck ();
void invertDirection();
void wait (unsigned long t);

unsigned long _lastReadTime = 0;
unsigned long _lastFlipTime = 0;
size_t        _lastStateRead;
size_t        _steps = 0;
size_t        _revolutions = 0;
boolean       _unstucking = false;
boolean       _goingBackward = false;
boolean       _isRunning  = false;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while(!Serial){}
  Serial.println("Device started");
  pinMode(BWD_PIN, OUTPUT);
  pinMode(FWD_PIN, OUTPUT);
  pinMode(OPT_PIN, INPUT);
  pinMode(BUT_PIN, INPUT);
  _lastStateRead = analogRead(OPT_PIN) < CUT_THRESHOLD ? CUT : THROUGH;
}

void loop () {
  if (machineStarted()) {
    if (!_isRunning) {
      startMachine();
      _isRunning = true;
    }
    // TODO Agregar un control de atasco por seguridad. Sin importar logcia, si no se leyo ningun step en 
    // un tiempo X parar la maquina y no hacer nada mas sin importar el input del boton (machineStarted())
    // Si pasa esto, para arrancar la maquina nuevamente sera necesario un reset del sistema
    if (_lastReadTime + READ_INTERVAL < millis()) {
      if (becameStuck()) {
        Serial.println("Machine is stuck. Reverse machine.");
        _steps = 0;
        _revolutions = 0;
        _unstucking = true;
        invertDirection();
        _lastFlipTime = millis();
      }
      if (_goingBackward && _revolutions >= 1) {
        Serial.println("Done reversing. Going forward.");
        _steps = 0;
        _revolutions = 0;
        invertDirection();
        _lastFlipTime = millis();
      }
      int read = analogRead(OPT_PIN);
      boolean flipped;
      boolean cut;
      if (read < CUT_THRESHOLD) {
        flipped = _lastStateRead == THROUGH;
        cut = true;
      } else {
        flipped = _lastStateRead == CUT;
        cut = false;
      }
      if (flipped) {
        _lastFlipTime = millis();
        if (!cut) {
          ++_steps;
          Serial.printf("Steps counted %d", _steps);
          Serial.println();
          if (_steps == STEPS_IN_REV) {
            _steps = 0;
            ++_revolutions;
            // If not reversing and more than one entire revolution was completed, the unstucking procedure must end
            _unstucking = !_goingBackward && _revolutions > 1;
            Serial.printf("Revolutions counted %d", _revolutions);
            Serial.println();
          }
        }
        _lastStateRead = _lastStateRead == CUT ? THROUGH : CUT;
      }
      _lastReadTime = millis();
    }
  } else {
    //Si la maquina no funciona, mantengo los contadores actualizados
    _lastFlipTime = millis();
    _lastReadTime = millis();
    if (_isRunning) {
      Serial.println("Stopping machine");
      stopMachine();
    }
    delay(500);
    _isRunning = false;
  }
}

void invertDirection() {
  digitalWrite(!_goingBackward ? FWD_PIN : BWD_PIN, LOW);
  Serial.print("Waiting machine to stop");
  wait(TURN_DELAY_SECONDS * 1000);
  digitalWrite(!_goingBackward ? BWD_PIN : FWD_PIN, LOW);
  _goingBackward = !_goingBackward;
  if (_goingBackward) {
    Serial.println("Now running backward");
  } else {
    Serial.println("Now running forward");
  }
}

void wait (unsigned long t) {
  long started = millis();
  while (millis() < started + t) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
}

void startMachine() {
  Serial.println("Starting machine");
  digitalWrite(FWD_PIN, HIGH);
}

void stopMachine () {
  Serial.println("Stopping machine");
  digitalWrite(FWD_PIN, LOW);
  digitalWrite(BWD_PIN, LOW);
}

boolean becameStuck() {
  return (_lastFlipTime + STUCK_THRESHOLD_SECONDS * 1000) < millis();
}

boolean machineStarted() {
  return true;
}
