#!/bin/bash

# Define the desired RF serial numbers
# Run within the same file as topology json
RF_SERIAL_NUMBERS=(
	    "RF3E000684"
            "RF3E000176"
            "RF3E000132"
            "RF3E000108"
            "RF3E000739"
            "RF3E000555"
            "RF3E000407"
            "RF3E000728"
            "RF3E000134"
            "RF3E000191"
            "RF3E000171"
            "RF3E000105"
            "RF3E000103"
            "RF3E000180"
            "RF3E000181"
            "RF3E000188"
            "RF3E000189"
            "RF3E000359"
            "RF3E000139"
            "RF3E000032"
            "RF3E000143"
            "RF3E000160"
            "RF3E000025"
            "RF3E000034"
            "RF3E000183"
            "RF3E000152"
            "RF3E000123"
            "RF3E000178"
            "RF3E000154"
            "RF3E000182"
            "RF3E000038"
            "RF3E000137"
    )


# Path to the JSON file
JSON_FILE="/scratch/repos/RENEWLab/CC/Sounder/files/topology-honors.json"

# Read the content of the JSON file and store it in a variable
JSON_CONTENT=$(cat "$JSON_FILE")

# Loop through each RF serial number and modify the JSON content
LAST_RF_NUMBER=$(awk -F '["\[]' '/"BaseStations": \{/{flag=1; next} /"sdr": \[/ && flag{getline; print $2; exit} /}/ && flag{exit}' topology-honors.json)
echo "Value within sdr array: $LAST_RF_NUMBER"
for RF_NUMBER in "${RF_SERIAL_NUMBERS[@]}"; do
    # Replace the existing RF serial number in the "sdr" array with the current one
    MODIFIED_JSON="${JSON_CONTENT//\"$LAST_RF_NUMBER\"/\"$RF_NUMBER\"}"

    echo "Last RF number: $LAST_RF_NUMBER"
    echo "RF number in loop: $RF_NUMBER"
    echo "$MODIFIED_JSON"
    # Write the modified JSON back to the file
    echo "$MODIFIED_JSON" > "$JSON_FILE"

    # Store the current RF number as the last RF number for the next iteration
    LAST_RF_NUMBER="$RF_NUMBER"

    echo "Modified $JSON_FILE with RF serial number: $RF_NUMBER"

    # Add a 1-second delay
    # sleep 1

    JSON_CONTENT=$(cat "$JSON_FILE")
done
