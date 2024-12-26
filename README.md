# Tram Schedule IoT Project

## Overview
The Tram Schedule IoT Project is a real-time tram schedule display system built using an ESP32 microcontroller, LED matrices, and a simple web-based interface. 
The system allows users to view tram schedules for selected stops, with the option to select a stop based on their current location. The project utilizes PHP to connect the frontend with the backend, ensuring a seamless user experience.

## Features 

- Real-Time Tram Schedules: Displays the schedule for a selected tram stop, fetched from an HSL API.
- Interactive Stop Selection: Users can choose a tram stop nearby Pasila Haaga-Helia campus from a list, and the schedule for that stop is shown on the display.
- ESP32 with LED Matrices: The ESP32 microcontroller controls two LED matrices to display the tram schedules clearly and efficiently.
- Web Interface: An OLED display allows users to scan a QR code to access the tram stop selection page.


## Technologies
- ESP32: The microcontroller used to fetch tram schedules, display data on LED matrices, and interact with the web backend.
- LED Matrices: Used to display tram schedules in a clear and easy-to-read format.
- PHP: Handles the communication between the frontend (web page) and the backend, managing tram stop data and schedule retrieval.
- HTML/CSS: Web technologies used to create the user interface for selecting tram stops.
- Git/GitHub: Version control for managing the project's codebase and collaboration

