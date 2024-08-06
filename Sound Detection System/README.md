This project was designed during my 2024 Spring semester. The purpose was to accurately detect the angle within a 5-10 degree error, which would be our ideal range of accuracy. To achieve this, a simple circuit using four CMC-9745-44P microphones was used to sample the audio signals into readable voltage levels. From there, using the ADC inputs provided by the TM4C123G board we can correlate which speaker was closest to the sound based on numerical readings. To facilitate our calcuations, the speakers are placed in equal distances with a 120 degree diffrence. The idea is that by finding the order of arrival, we can deduce the location of the sound and find the angle. 