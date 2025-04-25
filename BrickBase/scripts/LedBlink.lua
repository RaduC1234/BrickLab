print("Turning LED on GPIO 19 ON for 3 seconds...")

while true do
  led_on(19)
  delay(1000)
  led_off(19)
  delay(1000)
end

