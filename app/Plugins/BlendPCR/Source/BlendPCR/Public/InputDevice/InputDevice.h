#pragma once

#include<string>

class InputDevice {
public:
	virtual bool isValid() = 0;

	virtual std::string getIdentifier() = 0;
};