#include "html_content.hpp"

const std::string html_page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wi-Fi Configuration</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #1c1c1c; /* Dark gray background */
            color: white; /* White text */
            text-align: center;
            padding: 20px;
        }
        .container {
            background: #2a2a2a; /* Slightly lighter gray */
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0px 0px 10px rgba(255,255,255,0.1);
            width: 80%;
            max-width: 400px;
            margin: auto;
        }
        h1 {
            color: white;
        }
        label {
            font-size: 16px;
            display: block;
            text-align: left;
            margin: 10px 0 5px;
            color: white;
        }
        input {
            width: 100%;
            padding: 10px;
            margin-bottom: 15px;
            border: 1px solid #666;
            border-radius: 5px;
            background: #333; /* Darker input field */
            color: white;
        }
        button {
            background-color: #28a745;
            color: white;
            border: none;
            padding: 10px;
            width: 100%;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
        }
        button:hover {
            background-color: #218838;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Wi-Fi Configuration</h1>
        <form method='POST' action='/updateSettings'>
            <label for="ssid">Wi-Fi SSID:</label>
            <input type='text' id="ssid" name='ssid' required>

            <label for="password">Wi-Fi Password:</label>
            <input type='password' id="password" name='password' required>

            <button type='submit'>Save Wi-Fi Settings</button>
        </form>
    </div>
</body>
</html>
)rawliteral";
