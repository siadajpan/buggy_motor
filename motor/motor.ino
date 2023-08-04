#include <IoAbstraction.h>
#include <ExecWithParameter.h>


#define EN_1 5
#define DIR_1 6
#define PUL_1 7
#define EN_2 9
#define DIR_2 10
#define PUL_2 11
#define JOYSTICK 0
#define MAX_SPEED 60
#define JOYSTICK_CUT_OFF 50  //joystick shouldn't respond within those values
#define FREQ_CUT_OFF JOYSTICK_CUT_OFF*MAX_SPEED // frequency when the motor should just stop
#define FREQ_ACCELERATION 5

int x_val, x_dir = 0;
bool engine_enabled = true;
bool moving_forward = true;
long frequency = 0;
long frequency_target = 0;
int joystic_dead_zone = 200;
int delay_acc_us = 1000;
long delay_us = 0;

void engine_enable(bool enabled){
  digitalWrite(EN_1, enabled? LOW : HIGH);
  digitalWrite(EN_2, enabled? LOW: HIGH);
  engine_enabled = enabled;
}

void engine_direction(bool forward){
  digitalWrite(DIR_1, forward? HIGH : LOW);
  digitalWrite(DIR_2, forward? HIGH : LOW);
  moving_forward = forward;
}

class Motor: public Executable {
private:
  int enable_pin;
  int direction_pin;
  int pulse_pin;
  int frequency_target;
  int frequency;
  TaskManager* taskManager;
  bool enabled;
  bool moving_forward;

public:
  Motor(int enable_pin, int direction_pin, int pulse_pin, TaskManager* tm){
    enable_pin = enable_pin;
    direction_pin = direction_pin;
    pulse_pin = pulse_pin;
    taskManager = tm;
  }

  void enable(bool enabled){
    digitalWrite(enable_pin, enabled? LOW : HIGH);
  }
  void set_direction(bool forward) {
    digitalWrite(direction_pin, forward? HIGH : LOW);
  }
  
  void update_frequency_target(int new_frequency_target){
    frequency_target = new_frequency_target;
  }

  // void update_frequency_and_move(int frequency)

};

void update_frequency_and_move(){
  // update frequency according to the current and target
  int frequency_diff = frequency - frequency_target;
  if (abs(frequency_diff) >= FREQ_ACCELERATION) {
    if (frequency_diff > 0)
      frequency = max(frequency - FREQ_ACCELERATION, frequency_target);
    else 
      frequency = min(frequency + FREQ_ACCELERATION, frequency_target);
  }
  // Serial.print("frequency: ");
  // Serial.print(frequency);
  // stop motor if current frequency is around 0
  if (abs(frequency) <= FREQ_CUT_OFF){
    Serial.println("\n FREQ LESS THAN CUT OFF");
    engine_enable(false);
    return;
  }

  // motor will move, enable it if not yet
  if (!engine_enabled)
    Serial.println("\n ENABLING ENGINE");
    engine_enable(true);

  // update direction based on frequency sign
  if (frequency > 0 && !moving_forward){
    Serial.println("\n CHANGING DIRECTION -> FORWARD");
    engine_direction(true);
  }
  else if (frequency < 0 && moving_forward){
    Serial.println("\n CHANGING DIRECTION -> BACK");
    engine_direction(false);
  }
  
  // set delay between the pulses
  delay_us = 1000000 / abs(frequency);


  // make a step
  digitalWrite(PUL_1, HIGH);  
  digitalWrite(PUL_2, HIGH);  
  taskManager.schedule(onceMicros(int(delay_us/2)), [] {
      digitalWrite(PUL_1, LOW);   
      digitalWrite(PUL_2, LOW);   
    }
  );

  // run the same task with the same parameter again
  // auto task = new ExecWithParameter<int>(update_frequency_and_move, motor_id);
  taskManager.schedule(onceMicros(delay_us), update_frequency_and_move);
}

void read_joystick(){
  x_val = analogRead(JOYSTICK);
  x_dir = x_val - 507;

  Serial.print(" x: ");
  Serial.print(x_dir);

  // joystick moved, update target dir
  // TODO change this to update_frequency_target for each motor
  frequency_target = x_dir * MAX_SPEED;

  
  Serial.print(", freq: ");
  Serial.print(frequency);

  Serial.print(", freq_target: ");
  Serial.println(frequency_target);

  // joystick moved when engine was off
  if (abs(x_dir) > JOYSTICK_CUT_OFF && !engine_enabled) {

    Serial.println(" STARTING ");
    // set the starting frequency
    frequency = (x_dir > 0) ? FREQ_CUT_OFF : -FREQ_CUT_OFF;
    // auto go_left = new ExecWithParameter<int>(update_frequency_and_move, 0);
    // auto go_right = new ExecWithParameter<int>(update_frequency_and_move, 1);
    update_frequency_and_move();
    // taskManager.schedule(0, go_right);
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(EN_1, OUTPUT);
  pinMode(EN_2, OUTPUT);
  pinMode(DIR_1, OUTPUT);
  pinMode(DIR_2, OUTPUT);
  pinMode(PUL_1, OUTPUT);
  pinMode(PUL_2, OUTPUT);

  Serial.begin(9600);
  engine_enable(false);
  engine_direction(true);

  taskManager.scheduleFixedRate(100, read_joystick);
}

void loop() {

  taskManager.runLoop();

  
  // Serial.print("x: ");
  // Serial.print(x_dir);
  // Serial.print(", freq_target: ");
  // Serial.print(frequency_target);
  // Serial.print(", freq: ");
  // Serial.print(frequency);
  // Serial.print(", freq_diff: ");
  // Serial.print(frequency_diff);
  // Serial.print(", delay: ");
  // Serial.print(delay_us);
  // Serial.println("");

}
