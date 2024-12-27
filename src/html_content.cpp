#include "html_content.hpp"

const std::string html_page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Scale Config</title>
</head>
<body>
    <h1>Scale Configuration</h1>
    <form method='POST' action='/updateSettings'>
        <label>Set Weight: </label><input type='number' name='setWeight' step='0.1'><br><br>
        <label>Offset: </label><input type='number' name='offset' step='0.01'><br><br>
        <label>Scale Mode: </label><input type='checkbox' name='scaleMode'><br><br>
        <label>Grind Mode: </label><input type='checkbox' name='grindMode'><br><br>
        <button type='submit'>Update Settings</button>
    </form>
</body>
</html>
)rawliteral";
