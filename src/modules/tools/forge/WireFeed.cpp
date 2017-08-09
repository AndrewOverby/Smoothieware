///////////////////////////////////////////////////////////////////////////////////////////////////
/*																								 */
/*	WireFeed.cpp																				 */
/*	07 August 2017																				 */
/*	Matthew Buchanan																			 */
/*	Forgeware																					 */
/*																								 */
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "WireFeed.h"

#include "Module.h"
#include "Kernel.h"
#include "Gcode.h"
#include "PwmOut.h" // mbed.h lib
#include "Pin.h"
#include "StreamOutputPool.h"

#include <math.h>

#define FEEDPIN "2.5"

WireFeed::WireFeed() { 

	rate = 10000; 		//default wire feed rate in mm/min
	factor = 100.0F;
	period = 20000;		//not sure what the period is supposed to be, differing documentation
	feeding = 0;
}
void WireFeed::on_module_loaded() { 

	Pin* dummypin = new Pin;
	dummypin->from_string(FEEDPIN)->as_output();

	feed_pin = dummypin->hardware_pwm();

	if (feed_pin == NULL) {

		THEKERNEL->streams->printf("Error in pin initialization\n");
		delete dummypin;
		delete this;
		return;
	}
	delete dummypin;
	dummypin = NULL;

//	this->feed_pin->period_us(period);	//this gives the wrong frequency, default seems to be fine

	this->register_for_event(ON_GCODE_RECEIVED);
}
void WireFeed::on_gcode_received(void *argument) { 

	Gcode *gcode = static_cast<Gcode *>(argument);

    // M codes execute immediately
    if (gcode->has_m) {
        if (gcode->m == 750) { 
            if(gcode->has_letter('S')) { 				//this will be the wire feed rate in mmpm, will be a modal command
                this->rate = gcode->get_value('S');
            }
            this->feed_pin->write(pwm_duty_cycle()/100.0F); 	//need to put in the relationship between duty cycle and rate
            this->feeding = 1;
//			THEKERNEL->streams->printf("Wire Feed rate is %0.0f mm/min\nDuty Cycle set to %0.0f %%\n", rate * factor/100.0F, pwm_duty_cycle());
        }
        if (gcode->m == 760) {
        	this->feed_pin->write(0);
        	this->feeding = 0;
        }
        if (gcode->m == 755) { 							//this will be the wire feed rate override command
        	if(gcode->has_letter('S')) { 
        		this->factor += gcode->get_value('S');
				THEKERNEL->streams->printf("Wire Feed factor at %0.0f %%\n", factor);
				THEKERNEL->streams->printf("Wire Feed rate is %0.0f mm/min\n", rate * factor/100.0F);
        		if(this->feeding) {
        			this->feed_pin->write(pwm_duty_cycle()/100.0F);
        		} else {
        			this->feed_pin->write(0);
        		}
        	}
        }
    }
}
float WireFeed::pwm_duty_cycle() {
	
	//estimated as a fourth order polynomial (solved in MATLAB), not good enough
	float p1 = 1.0314 * pow(10, -13);
	float p2 = -2.0053 * pow(10, -9);
	float p3 = 1.413 * pow(10, -5);
	float p4 = -0.038416;
	float p5 = 43.607;
	
	return ((p1 * pow(rate, 4))+(p2 * pow(rate, 3))+(p3 * pow(rate, 2))+(p4 * rate)+p5) * factor/100.0F;
}