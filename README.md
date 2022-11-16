# WaterOrNot

## Table of contents
* [Introduction](#introduction)
* [Hardware requirements](#hardware-requirements)
* [Technologies](#technologies)
* [Setup](#setup)
* [To do](#to-do)

## Introduction
This project aims to predict when or when not to water your outdoor plants. It uses soil moisture content and weather data to predict a reasonable time to water plants. If the soil moisture content is high enough and rain is in the near future, it may be unwise to waste water.

## Hardware requirements
Looking at the problem it is very obvious to specify that we require a sensor capable of reading the soil moisture content, a controller that can access weather data, a power source and last but not least this all needs to be efficient and low power. Furthermore all this must be affordable and keep the final unit price low.

### Controller
Prior to beginning this project, the controller has already been chosen. The project will be using a Particle Argon development board kit, this microcontroller is 3V3 logic and has input for a LiPo battery as well as an onboard LiPo charger module.

### Sensor for soil moisture
Before researching sensors, let's specify a list of requirements
- **Supply voltage**: preferably 3V3 to avoid further switching losses
- **Range**: should be able to read in the range of completely dry to completely soaked soil (or just plain water)
- **Operating temperatures**: Outside temperatures during the summer, let's say 10°C to 40°C
- **Response time**: the sensor should be kept at a minimum to lower the power consumption during reads
- **Cost**: Low, this filters out any industrial-grade sensors
<!-- To do: Research and select sensor -->

### Power source
This project is going to be placed outside. For it to be mobile, we require a wireless power source. This could be anything from wind to solar or a button cell battery to a super capacitor.
As a starting point, this project will be using a LiPo battery. The reason behind this is because it's easy. I have a few laying around with capacities between 400 and 800 mAh and the microcontroller supports easy connection and charging of them. I will, however, like to set up a requirement which will later be determined if these batteries can fulfill it.
- **Capacity**: Should be able to supply the setup with power for at least two months
- **Replacable**: Definitely, this will allow you to keep it running, simply switch out with a fully charged battery

### Low power
Lorem


## Technologies
Project is created with:
* Particle Argon development board
* DS3231 RTC development board
* Capacitive soil moisture sensor

## Setup
TBD

### To do
Test:  
Power consumption:  
&nbsp;&nbsp;&nbsp;&nbsp;Ultra low power mode without DS3231  
&nbsp;&nbsp;&nbsp;&nbsp;vs.  
&nbsp;&nbsp;&nbsp;&nbsp;Hibernate and a DS3231  

Reliability of capacitive soil moisture sensor  
&nbsp;&nbsp;&nbsp;&nbsp;Are readings consistant?  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Do they fluctuate?  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Do they diminish over time due to wear?  
&nbsp;&nbsp;&nbsp;&nbsp;Is the range of the sensor adequate for the range dry soil to soaked soil?  
