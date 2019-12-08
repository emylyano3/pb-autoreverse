#include <Arduino.h>
// usable pins D0,D1,D2,D5,D6,D7 (D10 is TX (GPIO1), D9 is RX (GPIO3), D3 is GPIO0, D4 is GPIO2, D8 is GPIO15)
#ifdef NODEMCUV2
const int     PWR_SW_PIN    = D0; // power button switch
const int     BWD_SW_PIN    = D1; // backward button switch
const int     FWD_CMD_PIN   = D5; // forward command pin
const int     BWD_CMD_PIN   = D6; // backward command pin
const int     OPT_SEN_PIN   = D7; // optical sensor pin
const int     WAR_LED_PIN   = D2; // optical sensor pin

#elif ESP12
const int     PWR_SW_PIN    = 4;
const int     BWD_SW_PIN    = 5;
const int     FWD_CMD_PIN   = 12;
const int     BWD_CMD_PIN   = 14;
const int     OPT_SEN_PIN   = 16;
#endif

const size_t  CUT           = 0x00;
const size_t  THROUGH       = 0x01;

/* To know if the machine was started by user */
boolean runningMachine();
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
boolean       _startTriggered  = false;
boolean       _warningStop  = false;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while(!Serial){}
  Serial.println("Device started");
  pinMode(BWD_CMD_PIN, OUTPUT);
  pinMode(FWD_CMD_PIN, OUTPUT);
  pinMode(OPT_SEN_PIN, INPUT);
  pinMode(PWR_SW_PIN, INPUT);
  _lastStateRead = analogRead(OPT_SEN_PIN) < CUT_THRESHOLD ? CUT : THROUGH;
}

void loop () {
  if (runningMachine()) {
    if (!_startTriggered) {
      startMachine();
      _startTriggered = true;
    }
    // TODO Agregar un control de atasco por seguridad. Sin importar logica, si no se leyo ningun step en 
    // un tiempo X parar la maquina y no hacer nada mas sin importar el input del boton (runningMachine())
    // Si pasa esto, para arrancar la maquina nuevamente sera necesario un reset del sistema
    if (millis() > _lastFlipTime + WARNING_STOP_SECONDS * 1000) {
      _warningStop = true;
      digitalWrite(WAR_LED_PIN, HIGH);
    }
    if (_lastReadTime + READ_INTERVAL < millis()) {
      if (becameStuck()) {
        Serial.println("Machine is stuck. Reversing.");
        _steps = 0;
        _revolutions = 0;
        _unstucking = true;
        invertDirection();
        _lastFlipTime = millis();
      }
      if (_unstucking && _goingBackward && _revolutions >= 1) {
        Serial.println("Unstucking. Done reversing, now going forward.");
        _steps = 0;
        _revolutions = 0;
        invertDirection();
        _lastFlipTime = millis();
      }
      int read = analogRead(OPT_SEN_PIN);
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
            _unstucking = !_goingBackward;
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
    if (_startTriggered) {
      Serial.println("Stopping machine");
      stopMachine();
      _steps = 0;
      _revolutions = 0;
      _startTriggered = false;
      _goingBackward = false;
      _unstucking = false;
      _lastStateRead = analogRead(OPT_SEN_PIN) < CUT_THRESHOLD ? CUT : THROUGH;
    }
  }
  delay(500);
}

void invertDirection() {
  digitalWrite(_goingBackward ? BWD_CMD_PIN : FWD_CMD_PIN, LOW);
  Serial.print("Waiting machine to stop");
  wait(TURN_DELAY_SECONDS * 1000);
  digitalWrite(_goingBackward ? FWD_CMD_PIN : BWD_CMD_PIN, HIGH);
  _goingBackward = !_goingBackward;
  if (_goingBackward) {
    Serial.println("Now running backward");
  } else {
    Serial.println("Now running forward");
  }
}

void wait (unsigned long t) {
  long started = millis();
  unsigned long d = min(100UL, t/10);
  while (millis() < started + t) {
    Serial.print(".");
    delay(d);
  }
  Serial.println();
}

void startMachine() {
  Serial.println("Starting machine");
  digitalWrite(FWD_CMD_PIN, HIGH);
}

void stopMachine () {
  Serial.println("Stopping machine");
  digitalWrite(FWD_CMD_PIN, LOW);
  digitalWrite(BWD_CMD_PIN, LOW);
}

boolean becameStuck() {
  return (_lastFlipTime + STUCK_THRESHOLD_SECONDS * 1000) < millis();
}

boolean runningMachine() {
  return !_warningStop && LOW == digitalRead(PWR_SW_PIN);
}
