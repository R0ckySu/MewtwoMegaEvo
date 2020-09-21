//
// Created by Rocky Su on 18/9/20.
//

#include <istream>

/*
 * TimingBasic Class
 *
 * Foundamental abstracted class for all classes need timing logics.
 *
 * */

struct TimingDesc{
    std::string name;
    std::string table_head;
    std::string data_row;
};

class TimingBasic {
private:

public:
    double start_time = 0;
    double end_time = 0;
    double pulse_width = 0;

    double step_size;
    double total_num_steps;

    //Initializer
    TimingBasic();
    TimingBasic(double start, double end, double _step_size);
    //Destructor
    ~TimingBasic();
    //Copy constructor
    TimingBasic(const TimingBasic &t);

    virtual void set_pulse_width(double _pulse_width);
    virtual void shift_by_time(double shift_time);

    virtual double get_pulse_width();
    virtual int get_start_index();
    virtual int get_end_index();

    //Returns formatted table head and data string in pair. Subclasses are responsible to override this method!
    virtual TimingDesc description();
};
