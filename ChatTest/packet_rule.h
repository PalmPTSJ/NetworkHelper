#ifndef PACKET_RULE_H_INCLUDED
#define PACKET_RULE_H_INCLUDED

#include "network_core.h"

struct packet_rule {
    string rule;
    vector<int> ruleAddition; // store size data for string
    bool isChecksum;
    int id;
};

packet_rule packet_createRule(string rule,int id);
/**
d = int
c = char ( Byte )
[size]s = string

ex : ddc10s15s
*/
#endif // PACKET_RULE_H_INCLUDED
