

--code derived from
--http://www.instructables.com/id/ESP8266-Wifi-Temperature-Logger/

pin = 3
ow.setup(pin)

counter=0
lasttemp=-999

function bxor(a,b)
   local r = 0
   for i = 0, 31 do
      if ( a % 2 + b % 2 == 1 ) then
         r = r + 2^i
      end
      a = a / 2
      b = b / 2
   end
   return r
end





--- Get temp and send data to thingspeak.com
function sendData()

--initialize the temp and humidity variables
if(to==nil) then
  to=0
end

if(ho==nil) then
  ho=0
end

-- conection to thingspeak.com
--print("Sending data to thingspeak.com")
conn=net.createConnection(net.TCP, 0) 
conn:on("receive", function(conn, payload) print(payload) end)
-- api.thingspeak.com 184.106.153.149
conn:connect(80,'184.106.153.149') 
--replace YOUR-KEY with the API key for your thingspeak channel
conn:send("GET /update?key=YOUR-KEY&field1="..to.."&field2="..ho.." HTTP/1.1\r\n") 
conn:send("Host: api.thingspeak.com\r\n") 
conn:send("Accept: */*\r\n") 
conn:send("User-Agent: Mozilla/4.0 (compatible; esp8266 Lua; Windows NT 5.1)\r\n")
conn:send("\r\n")
conn:on("sent",function(conn)
                      print("Closing connection")
                      conn:close()
                  end)
conn:on("disconnection", function(conn)
                                print("Got disconnection...")
  end)
end

-- send data every 3 min to thing speak
tmr.alarm(0, 180000, 1, function() sendData() end )
