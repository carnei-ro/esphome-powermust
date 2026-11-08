import esphome.codegen as cg
from esphome.components import uart
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@syssi"]
DOMAIN = "powermust"
AUTO_LOAD = ["binary_sensor", "sensor", "switch", "text_sensor"]
MULTI_CONF = True

CONF_POWERMUST_ID = "powermust_id"
CONF_COMMAND_PREFIX = "command_prefix"
CONF_COMMAND_TERMINATOR = "command_terminator"

powermust_ns = cg.esphome_ns.namespace("powermust")
PowermustComponent = powermust_ns.class_("Powermust", cg.Component)

POWERMUST_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_POWERMUST_ID): cv.use_id(PowermustComponent),
    }
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PowermustComponent),
            # Optional bytes sent before every command. The Intelbras
            # XNB 720 BI needs a blank CR to wake up its parser.
            cv.Optional(CONF_COMMAND_PREFIX, default=""): cv.string_strict,
            # Megatec devices normally expect CR, but some firmwares (for
            # example the Intelbras XNB 720 BI) answer to CRLF as well.
            cv.Optional(CONF_COMMAND_TERMINATOR, default="\r"): cv.string_strict,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_command_prefix(config[CONF_COMMAND_PREFIX]))
    cg.add(var.set_command_terminator(config[CONF_COMMAND_TERMINATOR]))
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
