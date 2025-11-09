Hello World,

this is my own version of the SCS Telemetry with an integrated Websocket Server.

# How to install
It's easy, simply put the .dll File along the .ini File in your bin/win_x64/plugins of either ATS or ETS2.

# How to use
Simply connect with a websocket client to ws://localhost:9995 (You may edit the port in the .ini if you need to)
and once the game is running and you "approved" the SCS SDK Info Popup it will start streaming messages in JSON Format. Then you can implement it in your own app to show data and/or make a tracker like it was in my intention.

# FAQ
Q: Why is your code quality so gross?  
A: Mainly GitHub Copilot and OpenAI's ChatGPT did the work as I don't have any C++/C Knowledge myself.
  
  
Q: Do you plan on updating to new telemetry versions, if SCS decides to release new versions?  
A: Yes, of course. Whenever I have time I will to.  
  
Q: Why did you make this?  
A: I was fiddling around with TruckyApp's internal WebSocket Server and I very liked it to do something else  
than just doing Minecraft Plugins and expanding my horizont a bit. So I started messing around and the Idea to my own Telemetry was born.  
I wasted 4 weekends until I had a running plugin without errors and then I started working on the Tracker which I wrote in my language I have known for years now (Java). I currently am working to get a stable tracker for personal use / my VTC will be the beneficiary user under a license of my own.
  
  
Q: Do you plan to expand more [...]?  
A: Well, yes and no. I do have some ideas in mind indeed, but I learned my lesson - it's been hard but I will keep my best to keep working on it.  
  
Q: Where can we reach you in case of questions?  
A: Join my [Discord Guild](https://discord.gg/7XZ2AR9A9z)

# Dependencies
Last but not least, I used
- [SCS SDK](https://modding.scssoft.com/wiki/Documentation/Tools#Telemetry_SDK)
- [Nlohmann JSON](https://github.com/nlohmann/json)
- [Zaphoyd WebSocket++](https://github.com/zaphoyd/websocketpp)

As well I'd like to say that all Files are already included in the repository.  
The SCS SDK is in the - surprise - /scssdk folder,  
the json and websocketpp deps are in the /external folder.

# Forking, Improving and Sharing
Primarily I made this for personal use solely by AI Bots.  
I do allow forking and improving as well as sharing, as long as you mention me in some way - I'd appreciate that.  
If you improved my version and want me to have it as well, feel free to open a PR :)
