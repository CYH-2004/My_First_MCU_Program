# STC89C516
# A project of Gluttonous Snake，by using STC89C516RD+ to drive LCD ST12048A01.
# **************************************From CYH-2004*************************************************************
# It's the first time I learn to program MCU. And some basic function came from my dad (init,write_command,write_data...).
# Until 2024.3.5,I have spent about 2 weeks on learning how to program with C. I really learned a lot during programing.
# **************************************Description***************************************************************
# 2023.3.5
# Now ,the program can perfectly process the moving system of the snake.
# I wrote a bit-type coordinate system,which can calculate the snake's coordinates and recorde the status.
# The system is realized through the following steps:
# 1.System read the right number of target coordinates column where the snake head was located.
# 2.A 3-bit probe to scan the target coordinates column and return the results.(Repeat 3 times to scan all three "area")
# 3.Data comes from the probe decide which kind of picture to be displayed on the screen.
# 4.System calculate the location of snake tail.
# 5.The probe scan again and decide how to erase the tail.
# 6.Erasing the expired tail.
# In the next few days, I am going to make the program be able to create food randomly and limit the range.
