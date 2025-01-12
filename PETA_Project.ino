int led_red = 2;
int led_yellow = 3;
int led_green = 4;
int led_blue = 9;

int buzzer1 = A1;
int buzzer2 = A2;

int TRIG_PIN = 5;
int ECHO_PIN = 6;

int button1 = 7;
int button2 = 8;

bool suicides = false;
bool laps = false;
bool running = false;

unsigned long startMillis = 0;     // Start time for the run
unsigned long previousMillis = 0;  // Time of last lap
unsigned long lapTime = 0;         // Time taken for current lap
unsigned long totalTime = 0;       // Total time running

int lapCount = 0;             // To count laps
unsigned long lapTimes[100];  // Array to store lap times

float totalDistance = 0;  // Variable to store total distance covered
int suicidesLeft = 20;    // Remaining suicides to be completed in this session
int suicidesCompleted = 0;  // Total suicides completed in this session

void setup() {
  pinMode(led_red, OUTPUT);
  pinMode(led_yellow, OUTPUT);
  pinMode(led_green, OUTPUT);
  pinMode(led_blue, OUTPUT);

  pinMode(buzzer1, OUTPUT);
  pinMode(buzzer2, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);  // Set the Trig pin as output
  pinMode(ECHO_PIN, INPUT);   // Set the Echo pin as input

  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  Serial.begin(9600);  // Start the Serial Monitor
  
  // Initialize random seed using analogRead on an unused pin
  randomSeed(analogRead(A0));  // Use pin A0 to generate a random seed
}

void loop() {
  // Start suicides mode with button1
  if (digitalRead(button1) == HIGH && !running) {
    led_blink();
    suicides = true;
    laps = false;  // Clear laps flag
    running = true;
    resetSession();  // Reset session variables
    suicidesLeft = 20;  // Reset remaining suicides count to 20 for a new session
    suicidesCompleted = 0;  // Reset completed suicides
    digitalWrite(led_blue, HIGH);  // Turn on blue LED when running starts
    delay(1000);
  }

  // Start laps mode with button2
  else if (digitalRead(button2) == HIGH && !running) {
    led_blink();
    suicides = false;  // Clear suicides flag
    laps = true;
    running = true;
    resetSession();  // Reset session variables
    digitalWrite(led_blue, HIGH);  // Turn on blue LED when running starts
    delay(1000);
  }

  // Stop any mode with either button when running, and wait for release
  else if ((digitalRead(button1) == HIGH || digitalRead(button2) == HIGH) && running) {
    delay(500);  // Add a small delay to debounce and prevent immediate re-trigger
    if (digitalRead(button1) == LOW && digitalRead(button2) == LOW) {  // Wait for release
      running = false;
      digitalWrite(led_blue, LOW);  // Turn off blue LED when session stops
      printResults();  // Print results with the current suicides count
      suicides = false;
      laps = false;
      digitalWrite(led_red, LOW);
      digitalWrite(led_yellow, LOW);
      digitalWrite(led_green, LOW);
      delay(500);  // Prevent bouncing and allow state change to settle
    }
  }

  if (running) {
    sensor();  // Detect laps or suicides
  }
}

void resetSession() {
  startMillis = millis();                 // Start the overall timer
  previousMillis = startMillis;           // Initialize previous time for laps
  lapCount = 0;                           // Reset lap counter
  totalTime = 0;                          // Reset total time
  totalDistance = 0;                      // Reset total distance
  suicidesLeft = 20;                      // Reset remaining suicides count for a new session
  suicidesCompleted = 0;                  // Reset completed suicides count for a new session
  memset(lapTimes, 0, sizeof(lapTimes));  // Clear the lap times array (optional)
}

void printResults() {
  // Print the results after stopping
  if (suicides) {
    Serial.println("\n--- Suicides Mode ---");
    Serial.print("Total Time: ");
    Serial.print(totalTime / 1000.0, 2);  // Total time in seconds, 2 decimals
    Serial.println(" seconds");
    Serial.print("Total Distance: ");
    Serial.print(totalDistance, 2);  // Add total distance here, assume you store it
    Serial.println(" cm");
    Serial.print("Number of Suicides Completed: ");
    Serial.println(suicidesCompleted);
    Serial.print("Number of Suicides Left: ");
    Serial.println(suicidesLeft);
  } 
  else if (laps) {
    Serial.println("\n--- Laps Mode ---");
    
    // Calculate the total time based on the sum of all lap times
    unsigned long sumLapTimes = 0;
    for (int i = 0; i < lapCount; i++) {
      sumLapTimes += lapTimes[i];  // Add each lap time to the total sum
    }

    Serial.print("Total Time: ");
    Serial.print(sumLapTimes / 1000.0, 2);  // Sum of lap times in seconds, 2 decimals
    Serial.println(" seconds");

    // Print each lap time
    for (int i = 0; i < lapCount; i++) {
      Serial.print("Lap ");
      Serial.print(i + 1);  // Lap number (1-based index)
      Serial.print(": ");
      Serial.print(lapTimes[i] / 1000.0, 2);  // Lap time in seconds, 2 decimals
      Serial.println(" seconds");
    }
  }

  Serial.println("---------------------");
}


void sensor() {
  // Send a 10µs pulse to trigger the sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the Echo pin and calculate the distance
  long duration = pulseIn(ECHO_PIN, HIGH);  // Measure the time the echo takes
  float distance = (duration * 0.034) / 2;  // Convert to distance in cm

  // Update total time
  totalTime = millis() - startMillis;  // Calculate total time since start

  // Logic for Suicides Mode
  if (suicides) {
    static float randomDistance = 0;
    static bool directionAway = true;  // Flag to check if we're running away or back to the sensor

    // Trigger a random distance between 50-400 cm at the start or when the count reaches 0
    if (randomDistance == 0 || suicidesLeft == 0) {
      randomDistance = random(50, 401);  // Get a random distance between 50 and 400 cm
      Serial.print("Random distance set to: ");
      Serial.println(randomDistance);
    }

    if (directionAway) {
      // When running away from the sensor, check if distance exceeds the random distance
      if (distance >= randomDistance) {
        directionAway = false;  // Runner is now close enough, turn around
        Serial.println("Run back!");

        // Make buzzer1 beep for 25 ms to signal a turnaround
        digitalWrite(buzzer1, HIGH);
        delay(25);
        digitalWrite(buzzer1, LOW);
      }
    } else {
      // When running back to the sensor, check if the distance is less than 40 cm
      if (distance < 40) {
        directionAway = true;  // Runner is far enough away again, turn around
        suicidesLeft--;  // Decrease the remaining suicides count after completing one
        suicidesCompleted++;  // Increase the completed suicides count
        totalDistance += randomDistance;  // Add random distance to total distance
        Serial.print("Suicides left: ");
        Serial.println(suicidesLeft);

        // Make buzzer1 beep for 25 ms to signal a turnaround
        digitalWrite(buzzer1, HIGH);
        delay(25);
        digitalWrite(buzzer1, LOW);

        if (suicidesLeft <= 0) {
          // Once all suicides are done, play a final tune
          playFinalTune();
          running = false;  // Stop the session
          digitalWrite(led_blue, LOW);  // Turn off blue LED when session ends
          printResults();  // Print results with the current suicides completed
        }

        // Generate a new random distance for the next suicide
        randomDistance = random(50, 401);  
        Serial.print("New Random distance: ");
        Serial.println(randomDistance);
      }
    }
  }

  // Logic for Laps Mode
  if (laps) {
    static bool inLap = false;  // Track if a lap has been detected
    if (distance >= 20 && distance <= 100) {
      // When the runner is within a valid range (20-100 cm)
      if (!inLap && (millis() - previousMillis > 7000)) {  // Debounce to prevent false triggering
        inLap = true;  // Mark as in lap
        lapTime = millis() - previousMillis;  // Calculate lap time
        previousMillis = millis();  // Update last lap time
        totalTime = millis() - startMillis;  // Update total time

        // Store lap time
        if (lapCount < 100) {
          lapTimes[lapCount] = lapTime;
          lapCount++;
        }

        // Blink blue LED twice to signal lap completion
        for (int i = 0; i < 10; i++) {
          digitalWrite(led_blue, LOW);
          digitalWrite(buzzer1, HIGH);
          delay(10);
          digitalWrite(buzzer1, LOW);
          delay(100);
          digitalWrite(led_blue, HIGH);
          delay(100);
        }
        Serial.print("Lap ");
        Serial.println(lapCount);
      }
    } else {
      inLap = false;  // Reset lap detection state when out of range
    }
  }

  delay(10);  // Wait for a brief moment before the next reading
}

void playFinalTune() {
  // Add a tune to play when suicides mode ends
  for (int i = 0; i < 3; i++) {
    digitalWrite(buzzer1, HIGH);
    digitalWrite(buzzer2, HIGH);
    delay(200);
    digitalWrite(buzzer1, LOW);
    digitalWrite(buzzer2, LOW);
    delay(200);
  }
}

void led_blink() {
  digitalWrite(buzzer1, HIGH);
  delay(100);
  digitalWrite(buzzer1, LOW);
  digitalWrite(led_red, HIGH);
  delay(1000);
  digitalWrite(buzzer1, HIGH);
  delay(100);
  digitalWrite(buzzer1, LOW);
  digitalWrite(led_yellow, HIGH);
  delay(1000);
  digitalWrite(buzzer1, HIGH);
  delay(100);
  digitalWrite(buzzer1, LOW);
  digitalWrite(led_green, HIGH);
  delay(1000);

  digitalWrite(buzzer1, HIGH);
  digitalWrite(buzzer2, HIGH);
  digitalWrite(led_red, LOW);
  digitalWrite(led_yellow, LOW);
  digitalWrite(led_green, LOW);
  delay(1000);
  digitalWrite(buzzer1, LOW);
  digitalWrite(buzzer2, LOW);

  delay(1000);
}
