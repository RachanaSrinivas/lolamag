#include "MessageQueue.h"

class Conversion {
  public:
    String toStrWithDelim(Message message) {
      char msgString[50];

      sprintf(msgString, "%d;%d;%d;%s", message.NodeFrom, message.NodeTo, message.Id,  String(message.Content));
      Serial.println(String(msgString));
      return String(msgString);
    }

    Message toObject(String msg) {
      String msgs[4];
      int index = 0, prevIdx = 0;

      for (int i = 0; i < msg.length() + 1; i++) {
        if (msg[i] == ';' || msg[i] == '\0') {
          msgs[index] = msg.substring(prevIdx, i);
          index++;
          prevIdx = i + 1;
        }
        if (index == 4) break;
      }

      Message message;
      message.NodeFrom = atoi(String(msgs[0]).c_str());
      message.NodeTo = atoi(String(msgs[1]).c_str());
      message.Id = atoi(String(msgs[2]).c_str());
      message.Type = msgs[3].substring(0, 1);
      message.Content = msgs[3].substring(2);

      return message;
    }
};
