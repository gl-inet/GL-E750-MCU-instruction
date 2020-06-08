module("luci.controller.admin.mcu_interface", package.seeall)
 
function index()
    entry({"admin", "system", "MCU-interface"}, cbi("admin_system/mcu_interface"), _("MCU Interface"), 1)
end
