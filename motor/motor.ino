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

class Motor : public Executable {
private:
  int id;
  int enable_pin;
  int direction_pin;
  int pulse_pin;
  int frequency_target;
  int frequency;
  TaskManager* taskManager;
  bool enabled;
  bool moving_forward;

public:
  Motor(int id, int enable_pin, int direction_pin, int pulse_pin, TaskManager* tm){
    this->id = id;
    this->enable_pin = enable_pin;
    this->direction_pin = direction_pin;
    this->pulse_pin = pulse_pin;
    this->taskManager = tm;
  }

  void enable(bool enabled){
    digitalWrite(this->enable_pin, enabled? LOW : HIGH);
  }
  void set_direction(bool forward) {
    digitalWrite(this->direction_pin, forward? HIGH : LOW);
  }
  
  void update_frequency_target(int new_frequency_target){
    this->frequency_target = new_frequency_target;
  }

  // Function to make a step with the specified delay
  void make_step(int delay) {
    digitalWrite(this->pulse_pin, HIGH);
    // Capture the 'this' pointer in the lambda function
    auto task = new ExecWith2Parameters<int, bool>(digitalWrite, this->pulse_pin, LOW);

    taskManager->schedule(onceMicros(delay), task);
    // taskManager->schedule(
    //   onceMicros(delay), [this](){
    //     digitalWrite(this->pulse_pin, LOW);
    // }
    // );
  }

  void exec() override {
    x_val = analogRead(JOYSTICK);
    int x_dir = x_val - 507;

    Serial.print("Motor ");
    Serial.print(id);
    Serial.print(": x: ");
    Serial.print(x_dir);

    // joystick moved, update target dir
    // TODO change this to update_frequency_target for each motor
    this->frequency_target = x_dir * MAX_SPEED;
    
    Serial.print(", freq: ");
    Serial.print(frequency);

    Serial.print(", freq_target: ");
    Serial.println(frequency_target);

    // joystick moved when engine was off
    if (abs(x_dir) > JOYSTICK_CUT_OFF && !this->enabled) {

      Serial.print("\nMOTOR ");
      Serial.print(id);
      Serial.println(" STARTING ");
      // set the starting frequency
      this->frequency = (x_dir > 0) ? FREQ_CUT_OFF : -FREQ_CUT_OFF;
      // auto go_left = new ExecWithParameter<int>(update_frequency_and_move, 0);
      // auto go_right = new ExecWithParameter<int>(update_frequency_and_move, 1);
      update_frequency_and_move();
      // taskManager.schedule(0, go_right);
    }
  }

  void update_frequency_and_move(){
    int frequency_diff = this->frequency - this->frequency_target;
    if (abs(frequency_diff) >= FREQ_ACCELERATION) {
      if (frequency_diff > 0)
        frequency = max(this->frequency - FREQ_ACCELERATION, this->frequency_target);
      else 
        frequency = min(this->frequency + FREQ_ACCELERATION, this->frequency_target);
    }
    // Serial.print("frequency: ");
    // Serial.print(frequency);
    // stop motor if current frequency is around 0
    if (abs(this->frequency) <= FREQ_CUT_OFF){
      Serial.print("\nMOTOR ");
      Serial.print(id);
      Serial.println(" FREQ LESS THAN CUT OFF ");
      enable(false);
      return;
    }

    // motor will move, enable it if not yet
    if (!this->enabled)
      Serial.print("\nMOTOR ");
      Serial.print(id);
      Serial.println(" ENABLING ENGINE");
      enable(true);

    // update direction based on frequency sign
    if (this->frequency > 0 && !this->moving_forward){
      Serial.print("\nMOTOR ");
      Serial.print(id);
      Serial.println(" CHANGING DIRECTION -> FORWARD");
      set_direction(true);
    }
    else if (this->frequency < 0 && this->moving_forward){
      Serial.print("\nMOTOR ");
      Serial.print(id);
      Serial.println(" CHANGING DIRECTION -> BACK");
      set_direction(false);
    }
    
    // set delay between the pulses
    int delay_us = 1000000 / abs(this->frequency);
    make_step(int(delay_us/2));
    // auto task = new ExecWithParameter<bool>(update_frequency_and_move, false);

    this->taskManager->schedule(onceMicros(delay_us), [this](){
      update_frequency_and_move();
    });
    // this->taskManager->schedule(onceMicros(delay_us), update_frequency_and_move);
  }
};


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
  // engine_enable(false);
  // engine_direction(true);

  Motor motor1(0, EN_1, DIR_1, PUL_1, &taskManager);
  motor1.enable(false);
  motor1.set_direction(true);

  taskManager.scheduleFixedRate(100, &motor1);
}

void loop() {
  taskManager.runLoop();
}
