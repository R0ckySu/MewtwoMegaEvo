//
// Created by Rocky Su on 18/9/20.
//

#include "TimingBasic.h"
#include <iomanip>
#include "math.h"
#include <sstream>

/********Timing Basic*******/
void TimingBasic::set_pulse_width(double _pulse_width) {
    pulse_width = _pulse_width;
    end_time = start_time + _pulse_width;
};

double TimingBasic::get_pulse_width() {
    pulse_width = end_time - start_time;
    return pulse_width;
};

void TimingBasic::shift_by_time(double shift_time) {
    start_time += shift_time;
    end_time += shift_time;
}

TimingDesc TimingBasic::description() {
    TimingDesc desc;
    desc.name = "BasicObj--";
    desc.table_head = "|--StartTime-|--Duration--|--EndTime---|";

    std::stringstream descStream;
    descStream << "|";
    descStream << std::setw(12) << std::scientific << std::setprecision(4) << start_time << "|";
    descStream << std::setw(12) << std::scientific << std::setprecision(4) << get_pulse_width() << "|";
    descStream << std::setw(12) << std::scientific << std::setprecision(4) << end_time << "|";

    desc.data_row = descStream.str();
    return desc;
}

TimingBasic::TimingBasic(const TimingBasic &t) {
    start_time = t.start_time;
    end_time = t.end_time;
    pulse_width = t.pulse_width;

    step_size = t.step_size;
    total_num_steps = t.total_num_steps;
}

TimingBasic::TimingBasic() {
    start_time = 0;
    end_time = 0;
    pulse_width = 0;
    step_size = 1;
}

TimingBasic::~TimingBasic() = default;

int TimingBasic::get_start_index() {
    return round(start_time/step_size);
}

int TimingBasic::get_end_index() {
    return round(end_time/step_size);
}

TimingBasic::TimingBasic(double start, double end, double _step_size) {
    start_time = start;
    end_time = end;
    step_size = _step_size;
}




