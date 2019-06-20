#include <LowPower.h> //low power library
#include "DHT.h" //temperature sensor library
#include <SoftwareSerial.h> //serial communication for bluetooth

#define ble_vcc 8     //bluetooth +
#define ble_gnd 7     //bluetooth -
#define ble_TX 5      //bluetooth TX
#define ble_RX 6      //bluetooth RX
#define ble_status 4  // bluetooth status
#define actuatorUP 9
#define actuatorDOWN 10
#define test true
#define persian_duration 27000

SoftwareSerial ble(6, 5);

struct sleepStateMachine
{

  private:
    short numberOfSleep = 0;

  public:
    int getNumberOfSleep() {
      return numberOfSleep;
    }

    void increaseNumberOfsleep() {
      numberOfSleep++;
    };

    void resetNumberOfSleep() {
      numberOfSleep = 0;
    };

    void sleep()
    {
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
    };
};


void setUpBLE()
{

  pinMode(ble_vcc, OUTPUT);
  pinMode(ble_gnd, OUTPUT);
  digitalWrite(ble_vcc, HIGH);
  digitalWrite(ble_gnd, LOW);
  pinMode(ble_status, INPUT);
  ble.begin(9600);

}

void dismissBLE()
{

  digitalWrite(ble_vcc, LOW);
  digitalWrite(ble_gnd, LOW);
  pinMode(ble_vcc, INPUT);
  pinMode(ble_gnd, INPUT);
  pinMode(ble_status, INPUT);
  ble.end();

}

String readFromBLE()
{
  String response = ble.readString();
  response.trim();

  return response;
}

struct MessageParser
{
  private:

  String message;
  String parsedMessage;
  bool parsable = false;

  public:

  String getMessage()
  {
    return message;
    };
  
  void setMessage(String m)
  {
    message = m;
    parseMessage();
    };
  
  boolean isParsable() {return parsable;}
  
  String getParsedMessage(){return parsedMessage;}
  
  String parseMessage()
  {
    
    if(message == "")
    {
      parsedMessage = "Error";
      } 
    else
    {
      if(message == "up")
      {
        parsedMessage = "up";
        parsable = "true";
        }
        else if(message == "down")
        {
          parsedMessage = "down";
          parsable = "true";
          } 
          else if(message == "stop")
          {
            parsedMessage = "stop";
            parsable = "true";
            } 
            else
            {
              parsedMessage = "Error";
              parsable = "false";
              }
      }

      return parsedMessage;
    }
      
  void cleanParsedMessage()
  {
    message = "";
    parsedMessage = "";
    parsable = false;
    }
    
  };


///////////////// Actuators


void activateActuator(int actuator)
{
  digitalWrite(actuator, HIGH);
}

void deActivateActuator(int actuator)
{
  digitalWrite(actuator, LOW);
}


struct Actuators
{
  boolean actuatorsActive = false;
  String actuatorsState = "stop";

  void setActuator(String newState)
  {
    if(newState == "stop")
    {
      dismissActuators();
      actuatorsState = "stop";
      }
      else if (newState == "up" && actuatorsState != "up")
      {
        setUpActuators();
        activateActuator(actuatorUP);
        actuatorsState = "up";
        }
        else if (newState == "down" && actuatorsState != "down")
        {
          setUpActuators();
          activateActuator(actuatorDOWN);
          actuatorsState = "down";
          }
    }

  void setUpActuators()
  {

    pinMode(actuatorUP, OUTPUT);
    pinMode(actuatorDOWN, OUTPUT);
    digitalWrite(actuatorUP, LOW);
    digitalWrite(actuatorDOWN, LOW);

  }

  void dismissActuators()
  {

    digitalWrite(actuatorUP, LOW);
    digitalWrite(actuatorDOWN, LOW);
    pinMode(actuatorUP, INPUT);
    pinMode(actuatorDOWN, INPUT);
    actuatorsState = "stop";
  }
};

////////////////////Structs instances

struct sleepStateMachine sleepSM;
struct MessageParser messageParser;
struct Actuators persianActuator;

////////////////////SET UP

void setup() {

  if (test) {
    Serial.begin(9600); //uncomment to compare with serial monitor
  }

}

////////////////////LOOP

void loop()
{
  if (sleepSM.getNumberOfSleep() != 4)
  {

    //////////////// TEST
    ////// Writing information to serial port for tests purpose

    if (test)
    {
      Serial.println("Stil time to sleep");
      delay(300);
    }

    //////////////// PRODUCTION

    sleepSM.increaseNumberOfsleep();
    sleepSM.sleep();
  }
  else
  {
    /////////////// TEST
    ////// Writing information to serial port for tests purpose
    if (test)
    {
      Serial.println("I woke up!");
      delay(300);
    }

    ///////////// PRODUCTION

    setUpBLE();
    unsigned long timeBLEUp = millis();

    while ((millis() - timeBLEUp) < 3000)
    {
      if (ble.available() > 0) {

        delay(200); // Let message to fully arrive to ble
  
        messageParser.setMessage(readFromBLE());

        if (test)
        {
          Serial.println("Message is Parsable?: " + messageParser.isParsable());
          Serial.println("Message is: " + messageParser.getMessage());
          Serial.println("Message parsed is: " + messageParser.getParsedMessage());
          delay(100);
        }

        if(messageParser.isParsable())
        {
          ble.write("Ok");
          String newStateActuator = messageParser.getParsedMessage();
          persianActuator.setActuator(newStateActuator);          
          messageParser.cleanParsedMessage();

          unsigned long timeActuatorUp = millis();

          while(( millis() - timeActuatorUp ) < persian_duration){

            if (ble.available() > 0) {
                delay(200); // Let message to fully arrive to ble      
                messageParser.setMessage(readFromBLE());
                if(messageParser.isParsable())
                {
                  ble.write("Ok");

                  String newStateActuator = messageParser.getParsedMessage();
  
                  if (test)
                  {
                    Serial.println("Message while actuating is Parsable?: " + (String)messageParser.isParsable());
                    Serial.println("Message while actuating is Parsable is: " + messageParser.getMessage());
                    Serial.println("Message while actuating is Parsable parsed is: " + messageParser.getParsedMessage());
                    delay(100);
                  }
                  
                  persianActuator.setActuator(newStateActuator);
                  if(newStateActuator == "stop")
                  {
                    timeActuatorUp = millis()-23000; // if the message is stop, wait for other 5 seconds for a new message
                    }
                    else
                    {
                      timeActuatorUp = millis();
                      }
                  messageParser.cleanParsedMessage();
                }
                else
                {
                  ble.write("Error");
                  messageParser.cleanParsedMessage();
                  }
              } 
             }
             persianActuator.dismissActuators();
          }
          else
          {
            ble.write("Error");
            messageParser.cleanParsedMessage();
            }
      }
    }

    dismissBLE();
    sleepSM.resetNumberOfSleep();
    Serial.println("Number of Sleep resetted, time sleep again");

  }
}
//
//  Serial.println("Start to wait for a connection..." + String(millis()));
//
//
//  unsigned long time1 = millis();
//  while (digitalRead(ble_status) != HIGH)
//  {
//  }
//  ble.write("Hello!");
//  Serial.println("Connection established! time necessary: " + String(millis() - time1));
//  delay(1000);
//  if (ble.available() > 0) {
//    delay(200);  //wait that all characters are arrived
//    String bleInput = ble.readString();
//    bleInput.trim();
//
//    //Serial.println("abbiamo rivevuto come stringa dalla porta seriale: \"" + bleInput + "\"");
//    if (bleInput == "up")
//    {
//
//      ble.write("Roger");
//      digitalWrite(9, LOW);
//      digitalWrite(10, HIGH);
//    } else if (bleInput == "down")
//    {
//      ble.write("Roger");
//      digitalWrite(10, LOW);
//      digitalWrite(9, HIGH);
//    } else if (bleInput == "stop")
//    {
//      ble.write("Roger");
//      digitalWrite(9, LOW);
//      digitalWrite(10, LOW);
//    } else
//    {
//      ble.write("In che senso?");
//    }
//    delay(100); // wait that every is come
//  }
//
//
//  //digitalWrite(9,LOW);
//  //digitalWrite(10,LOW);
//
//  //switching off ble
//  pinMode(ble_gnd, INPUT);
//
//  //switching off ATMega
//  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
//
//  //switching bluetooth on again;
//  pinMode(ble_gnd, OUTPUT);

