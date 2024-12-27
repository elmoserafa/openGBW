#include "html_content.hpp"

const std::string html_page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Scale Config</title>
    <script>
        // Function to handle settings update
        async function updateSettings() {
            // Collect data from the form
            const data = {
                setWeight: parseFloat(document.querySelector('input[name="setWeight"]').value) || 0,
                offset: parseFloat(document.querySelector('input[name="offset"]').value) || 0,
                scaleMode: document.querySelector('input[name="scaleMode"]').checked,
                grindMode: document.querySelector('input[name="grindMode"]').checked
            };

            // Send data as a POST request with JSON payload
            try {
                const response = await fetch('/updateSettings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });

                const result = await response.json();
                alert(result.message || 'Error updating settings');
            } catch (error) {
                console.error('Error:', error);
                alert('Failed to update settings');
            }
        }
    </script>
</head>
<body>
    <h1>Scale Configuration</h1>
    <form onsubmit="event.preventDefault(); updateSettings();">
        <label>Set Weight: </label><input type='number' name='setWeight' step='0.1'><br><br>
        <label>Offset: </label><input type='number' name='offset' step='0.01'><br><br>
        <label>Scale Mode: </label><input type='checkbox' name='scaleMode'><br><br>
        <label>Grind Mode: </label><input type='checkbox' name='grindMode'><br><br>
        <button type='submit'>Update Settings</button>
    </form>
</body>
</html>
)rawliteral";
