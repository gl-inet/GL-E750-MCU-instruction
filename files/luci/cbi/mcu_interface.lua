m = Map("mcu", "MCU Interface")

g = m:section(NamedSection,"global","global","Global")

g:option(Flag, "hide_psk","Hide password", "Whether or not to show the WI-FI password in the LCD")
g:option(Flag, "screen1_en","Show screen1","Whether or not to show the first (main) screen.").default = "1"
g:option(Flag, "screen2_en","Show screen2","Whether or not to show the second (2.4G WI-FI info) screen.").default = "1"
g:option(Flag, "screen3_en","Show screen3","Whether or not to show the third (5G WI-FI info) screen.").default = "1"
g:option(Flag, "screen4_en","Show screen4","Whether or not to show the forth (LAN IP info) screen.").default = "1"
g:option(Flag, "screen5_en","Show screen5","Whether or not to show the fifth (VPN info) screen.").default = "1"
g:option(Flag, "debug","Debug information","Whether or not to  print debug information to system log.")
g:option(Flag, "custom_en","Customized","The content of the user customized screen.You can use a string up to 64 characters")

content = g:option(Value, "content","Content","Customized content.")
content.maxlength = 64
content:depends ("custom_en", "1")


return m -- Returns the map