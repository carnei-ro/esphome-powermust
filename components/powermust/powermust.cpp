#include "powermust.h"
#include "esphome/core/log.h"

// Fallback for ESPHome < 2026.10.0
#ifndef ESPHOME_LOG_TAG
#define ESPHOME_LOG_TAG(name, tag) static const char *const name = tag
#endif

namespace esphome::powermust {

ESPHOME_LOG_TAG(TAG, "powermust");

void Powermust::setup() {
  this->state_ = STATE_IDLE;
  this->command_start_millis_ = 0;
}

void Powermust::empty_uart_buffer_() {
  uint8_t byte;
  while (this->available()) {
    this->read_byte(&byte);
  }
}

void Powermust::loop() {
  // Read message
  if (this->state_ == STATE_IDLE) {
    this->empty_uart_buffer_();
    switch (this->send_next_command_()) {
      case 0:
        // no command send (empty queue) time to poll
        if (millis() - this->last_poll_ > this->update_interval_) {
          this->send_next_poll_();
          this->last_poll_ = millis();
        }
        return;
        break;
      case 1:
        // command send
        return;
        break;
    }
  }
  if (this->state_ == STATE_COMMAND_COMPLETE) {
    if (this->read_pos_ > 0) {
      ESP_LOGE(TAG, "Command not successful");
    } else {
      ESP_LOGI(TAG, "Command successful");
    }
    this->command_queue_[this->command_queue_position_] = std::string("");
    this->command_queue_position_ = (command_queue_position_ + 1) % COMMAND_QUEUE_LENGTH;
    this->state_ = STATE_IDLE;
  }

  if (this->state_ == STATE_POLL_DECODED) {
    switch (this->used_polling_commands_[this->last_polling_command_].identifier) {
      case POLLING_Q1:
        if (this->grid_voltage_) {
          this->grid_voltage_->publish_state(value_grid_voltage_);
        }
        if (this->grid_fault_voltage_) {
          this->grid_fault_voltage_->publish_state(value_grid_fault_voltage_);
        }
        if (this->ac_output_voltage_) {
          this->ac_output_voltage_->publish_state(value_ac_output_voltage_);
        }
        if (this->ac_output_load_percent_) {
          this->ac_output_load_percent_->publish_state(value_ac_output_load_percent_);
        }
        if (this->grid_frequency_) {
          this->grid_frequency_->publish_state(value_grid_frequency_);
        }
        if (this->battery_voltage_) {
          this->battery_voltage_->publish_state(value_battery_voltage_);
        }
        if (this->temperature_) {
          this->temperature_->publish_state(value_temperature_);
        }
        if (this->utility_fail_) {
          this->utility_fail_->publish_state(value_utility_fail_ == 1);
        }
        if (this->battery_low_) {
          this->battery_low_->publish_state(value_battery_low_ == 1);
        }
        if (this->bypass_active_) {
          this->bypass_active_->publish_state(value_bypass_active_ == 1);
        }
        if (this->ups_failed_) {
          this->ups_failed_->publish_state(value_ups_failed_ == 1);
        }
        if (this->ups_type_standby_) {
          this->ups_type_standby_->publish_state(value_ups_type_standby_ == 1);
        }
        if (this->test_in_progress_) {
          this->test_in_progress_->publish_state(value_test_in_progress_ == 1);
        }
        if (this->quick_test_switch_) {
          this->quick_test_switch_->publish_state(value_test_in_progress_ == 1);
        }
        if (this->deep_test_switch_) {
          this->deep_test_switch_->publish_state(value_test_in_progress_ == 1);
        }
        if (this->ten_minutes_test_switch_) {
          this->ten_minutes_test_switch_->publish_state(value_test_in_progress_ == 1);
        }
        if (this->shutdown_active_) {
          this->shutdown_active_->publish_state(value_shutdown_active_ == 1);
        }
        if (this->beeper_on_) {
          this->beeper_on_->publish_state(value_beeper_on_ == 1);
        }
        if (this->beeper_switch_) {
          this->beeper_switch_->publish_state(value_beeper_on_ == 1);
        }
        this->state_ = STATE_IDLE;
        break;
      case POLLING_F:
        if (this->ac_output_rating_voltage_) {
          this->ac_output_rating_voltage_->publish_state(value_ac_output_rating_voltage_);
        }
        if (this->ac_output_rating_current_) {
          this->ac_output_rating_current_->publish_state(value_ac_output_rating_current_);
        }
        if (this->battery_rating_voltage_) {
          this->battery_rating_voltage_->publish_state(value_battery_rating_voltage_);
        }
        if (this->ac_output_rating_frequency_) {
          this->ac_output_rating_frequency_->publish_state(value_ac_output_rating_frequency_);
        }
        this->state_ = STATE_IDLE;
        break;
    }
  }

  if (this->state_ == STATE_POLL_CHECKED) {
    char *tmp = reinterpret_cast<char *>(this->read_buffer_);
    switch (this->used_polling_commands_[this->last_polling_command_].identifier) {
      case POLLING_Q1: {
        ESP_LOGD(TAG, "Decode Q1");
        // (232.9 232.9 232.9 003 49.9 13.4 25.0 00001000\r
        // (005.2 232.4 226.8 002 50.1 12.9 25.0 10001000\r
        //                                 1  2  3  4  5  6  7  8
        char *start = strchr(tmp, '(');
        if (start == nullptr) {
          ESP_LOGD(TAG, "Q1 response has no '(' marker: %s", tmp);
          this->state_ = STATE_IDLE;
          break;
        }
        char *p = start + 1;  // skip '('
        value_grid_voltage_ = strtof(p, &p);
        value_grid_fault_voltage_ = strtof(p, &p);
        value_ac_output_voltage_ = strtof(p, &p);
        value_ac_output_load_percent_ = strtol(p, &p, 10);
        value_grid_frequency_ = strtof(p, &p);
        value_battery_voltage_ = strtof(p, &p);
        value_temperature_ = strtof(p, &p);
        while (*p == ' ')
          p++;
        char *flags_start = p;
        if (strlen(flags_start) < 8) {
          ESP_LOGD(TAG, "Q1 response has incomplete flags: %s", flags_start);
          this->state_ = STATE_IDLE;
          break;
        }
        value_utility_fail_ = *p++ - '0';
        value_battery_low_ = *p++ - '0';
        value_bypass_active_ = *p++ - '0';
        value_ups_failed_ = *p++ - '0';
        value_ups_type_standby_ = *p++ - '0';
        value_test_in_progress_ = *p++ - '0';
        value_shutdown_active_ = *p++ - '0';
        value_beeper_on_ = *p - '0';
        if (this->last_q1_) {
          this->last_q1_->publish_state(start);
        }
        if (this->flags_) {
          this->flags_->publish_state(std::string(flags_start, 8));
        }
        this->state_ = STATE_POLL_DECODED;
        break;
      }
      case POLLING_F: {
        ESP_LOGD(TAG, "Decode F");
        // "#220.0 003 12.00 50.0\r"
        char *start = strchr(tmp, '#');
        if (start == nullptr) {
          ESP_LOGD(TAG, "F response has no '#' marker: %s", tmp);
          this->state_ = STATE_IDLE;
          break;
        }
        char *p = start + 1;  // skip '#'
        value_ac_output_rating_voltage_ = strtof(p, &p);
        // Some firmwares report the current as "003" and others as "3.0".
        value_ac_output_rating_current_ = strtof(p, &p);
        value_battery_rating_voltage_ = strtof(p, &p);
        value_ac_output_rating_frequency_ = strtof(p, &p);
        if (this->last_f_) {
          this->last_f_->publish_state(start);
        }
        this->state_ = STATE_POLL_DECODED;
        break;
      }
      default:
        this->state_ = STATE_IDLE;
        break;
    }
    return;
  }

  if (this->state_ == STATE_POLL_COMPLETE) {
    if (this->read_buffer_[0] == '(' && this->read_buffer_[1] == 'N' && this->read_buffer_[2] == 'A' &&
        this->read_buffer_[3] == 'K') {
      this->state_ = STATE_IDLE;
      return;
    }
    this->state_ = STATE_POLL_CHECKED;
    return;
  }

  if (this->state_ == STATE_COMMAND || this->state_ == STATE_POLL) {
    while (this->available()) {
      uint8_t byte;
      this->read_byte(&byte);

      if (this->read_pos_ >= POWERMUST_READ_BUFFER_LENGTH - 1) {
        this->read_pos_ = 0;
        this->empty_uart_buffer_();
      }
      this->read_buffer_[this->read_pos_] = byte;
      this->read_pos_++;

      // end of answer (CR, LF or CRLF)
      if (byte == 0x0D || byte == 0x0A) {
        this->read_buffer_[this->read_pos_] = 0;
        this->empty_uart_buffer_();
        if (this->state_ == STATE_POLL) {
          this->state_ = STATE_POLL_COMPLETE;
        }
        if (this->state_ == STATE_COMMAND) {
          this->state_ = STATE_COMMAND_COMPLETE;
        }
      }
    }  // available
  }
  if (this->state_ == STATE_COMMAND) {
    // Wait for a negative response. On success the UPS doesn't respond
    if (millis() - this->command_start_millis_ > esphome::powermust::Powermust::COMMAND_TIMEOUT) {
      this->command_start_millis_ = millis();
      this->state_ = STATE_COMMAND_COMPLETE;
      return;
    }
  }
  if (this->state_ == STATE_POLL) {
    if (millis() - this->command_start_millis_ > esphome::powermust::Powermust::COMMAND_TIMEOUT) {
      // command timeout
      ESP_LOGD(TAG, "timeout command to poll: %s", this->used_polling_commands_[this->last_polling_command_].command);
      this->state_ = STATE_IDLE;
    }
  }
}

uint8_t Powermust::check_incoming_length_(uint8_t length) {
  if (this->read_pos_ - 3 == length) {
    return 1;
  }
  return 0;
}

// send next command used
uint8_t Powermust::send_next_command_() {
  if (!this->command_queue_[this->command_queue_position_].empty()) {
    const char *command = this->command_queue_[this->command_queue_position_].c_str();
    uint8_t byte_command[16];
    uint8_t length = this->command_queue_[this->command_queue_position_].length();
    for (uint8_t i = 0; i < length; i++) {
      byte_command[i] = (uint8_t) this->command_queue_[this->command_queue_position_].at(i);
    }
    this->state_ = STATE_COMMAND;
    this->command_start_millis_ = millis();
    this->empty_uart_buffer_();
    this->read_pos_ = 0;
    this->write_str(this->command_prefix_.c_str());
    this->write_str(command);
    // end Bytes
    this->write_str(this->command_terminator_.c_str());
    ESP_LOGD(TAG, "Sending command from queue: %s with prefix length %u and terminator length %u",
             command, this->command_prefix_.length(), this->command_terminator_.length());
    return 1;
  }
  return 0;
}

void Powermust::send_next_poll_() {
  this->last_polling_command_ = (this->last_polling_command_ + 1) % 15;
  if (this->used_polling_commands_[this->last_polling_command_].length == 0) {
    this->last_polling_command_ = 0;
  }
  if (this->used_polling_commands_[this->last_polling_command_].length == 0) {
    // no command specified
    return;
  }
  this->state_ = STATE_POLL;
  this->command_start_millis_ = millis();
  this->empty_uart_buffer_();
  this->read_pos_ = 0;
  this->write_str(this->command_prefix_.c_str());
  this->write_array(this->used_polling_commands_[this->last_polling_command_].command,
                    this->used_polling_commands_[this->last_polling_command_].length);
  // end Bytes
  this->write_str(this->command_terminator_.c_str());
  ESP_LOGD(TAG, "Sending polling command : %s with prefix length %u and terminator length %u",
           this->used_polling_commands_[this->last_polling_command_].command,
           this->command_prefix_.length(), this->command_terminator_.length());
}

void Powermust::queue_command_(const char *command, uint8_t length) {
  uint8_t next_position = command_queue_position_;
  for (uint8_t i = 0; i < COMMAND_QUEUE_LENGTH; i++) {
    uint8_t testposition = (next_position + i) % COMMAND_QUEUE_LENGTH;
    if (command_queue_[testposition].empty()) {
      command_queue_[testposition] = command;
      ESP_LOGD(TAG, "Command queued successfully: %s with length %u at position %d", command,
               command_queue_[testposition].length(), testposition);
      return;
    }
  }
  ESP_LOGD(TAG, "Command queue full dropping command: %s", command);
}

void Powermust::switch_command(const std::string &command) {
  ESP_LOGD(TAG, "got command: %s", command.c_str());
  queue_command_(command.c_str(), command.length());
}
void Powermust::dump_config() {
  ESP_LOGCONFIG(TAG, "Powermust:");
  ESP_LOGCONFIG(TAG, "Command prefix length: %u", this->command_prefix_.length());
  ESP_LOGCONFIG(TAG, "Command terminator length: %u", this->command_terminator_.length());
  ESP_LOGCONFIG(TAG, "used commands:");
  for (auto &used_polling_command : this->used_polling_commands_) {
    if (used_polling_command.length != 0) {
      ESP_LOGCONFIG(TAG, "%s", used_polling_command.command);
    }
  }

  LOG_SENSOR("", "Grid Voltage", this->grid_voltage_);
  LOG_SENSOR("", "Grid Fault Voltage", this->grid_fault_voltage_);
  LOG_SENSOR("", "AC Output Voltage", this->ac_output_voltage_);
  LOG_SENSOR("", "AC Output Load Percent", this->ac_output_load_percent_);
  LOG_SENSOR("", "Grid Frequency", this->grid_frequency_);
  LOG_SENSOR("", "Battery Voltage", this->battery_voltage_);
  LOG_SENSOR("", "Temperature", this->temperature_);
  LOG_SENSOR("", "AC Output Rating Voltage", this->ac_output_rating_voltage_);
  LOG_SENSOR("", "AC Output Rating Current", this->ac_output_rating_current_);
  LOG_SENSOR("", "Battery Rating Voltage", this->battery_rating_voltage_);
  LOG_SENSOR("", "AC Output Rating Frequency", this->ac_output_rating_frequency_);

  LOG_BINARY_SENSOR("", "Utility Fail", this->utility_fail_);
  LOG_BINARY_SENSOR("", "Battery Low", this->battery_low_);
  LOG_BINARY_SENSOR("", "Bypass Active", this->bypass_active_);
  LOG_BINARY_SENSOR("", "UPS Failed", this->ups_failed_);
  LOG_BINARY_SENSOR("", "UPS Type Standby", this->ups_type_standby_);
  LOG_BINARY_SENSOR("", "Test In Progress", this->test_in_progress_);
  LOG_BINARY_SENSOR("", "Shutdown Active", this->shutdown_active_);
  LOG_BINARY_SENSOR("", "Beeper On", this->beeper_on_);

  LOG_TEXT_SENSOR("", "Last Q1", this->last_q1_);
  LOG_TEXT_SENSOR("", "Flags", this->flags_);
  LOG_TEXT_SENSOR("", "Last F", this->last_f_);
}
void Powermust::update() {}

void Powermust::add_polling_command_(const char *command, ENUMPollingCommand polling_command) {
  for (auto &used_polling_command : this->used_polling_commands_) {
    if (used_polling_command.length == strlen(command)) {
      uint8_t len = strlen(command);
      if (memcmp(used_polling_command.command, command, len) == 0) {
        return;
      }
    }
    if (used_polling_command.length == 0) {
      size_t length = strlen(command) + 1;
      const char *beg = command;
      const char *end = command + length;
      used_polling_command.command = new uint8_t[length];  // NOLINT(cppcoreguidelines-owning-memory)
      size_t i = 0;
      for (; beg != end; ++beg, ++i) {
        used_polling_command.command[i] = (uint8_t) (*beg);
      }
      used_polling_command.errors = 0;
      used_polling_command.identifier = polling_command;
      used_polling_command.length = length - 1;
      return;
    }
  }
}

}  // namespace esphome::powermust
