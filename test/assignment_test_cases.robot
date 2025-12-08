*** Settings ***
Documentation     A self-contained test suite for the Automotive AC Control Panel.
...               Mocks the hardware keywords based on "code.ino" logic.

*** Variables ***
${DEFAULT_TARGET_TEMP}    24
${TURBO_TARGET_TEMP}      16
${EMERGENCY_TEMP_LIMIT}   50

*** Keywords ***
# --- Mocked Hardware & Logic Keywords ---

Power On System
    [Documentation]    Simulates System_Init() and default variables.
    Log To Console    --- Test Setup ---
    Log To Console    MOCK: System Initializing... LCD Init, ADC Init, Motor Init.
    # Initialize internal state variables to match C code defaults
    Set Suite Variable    ${CURRENT_TARGET}    ${DEFAULT_TARGET_TEMP}
    Set Suite Variable    ${CURRENT_ROOM_TEMP}    24
    Set Suite Variable    ${IS_TURBO}    ${False}
    Set Suite Variable    ${IS_EMERGENCY}    ${False}
    Set Suite Variable    ${FAN_SPEED}    0
    Set Suite Variable    ${FAN_DIR}      OFF
    Set Suite Variable    ${MODE_STR}     IDLE

Set Room Temperature Sensor
    [Arguments]    ${celcius}
    [Documentation]    Simulates the LM35 sensor reading (Get_Temperature).
    Set Suite Variable    ${CURRENT_ROOM_TEMP}    ${celcius}
    Log To Console    MOCK: Sensor reads room temperature: ${celcius} C
    _Update System State

Press Keypad Button
    [Arguments]    ${key_name}
    [Documentation]    Simulates Get_Keypad_Press() and switch logic.
    Log To Console    MOCK: User pressed button ${key_name}
    
    # Logic from C Code: Key Processing
    IF    "${key_name}" == "KEY_RIGHT"
        # Toggle Turbo Mode
        IF    ${IS_TURBO} == ${True}
            Set Suite Variable    ${IS_TURBO}    ${False}
            Set Suite Variable    ${CURRENT_TARGET}    24
        ELSE
            Set Suite Variable    ${IS_TURBO}    ${True}
            Set Suite Variable    ${CURRENT_TARGET}    16
        END
    ELSE IF    "${key_name}" == "KEY_UP"
        ${new_temp}=    Evaluate    ${CURRENT_TARGET} + 1
        Set Suite Variable    ${CURRENT_TARGET}    ${new_temp}
        Set Suite Variable    ${IS_TURBO}    ${False}
    ELSE IF    "${key_name}" == "KEY_DOWN"
        ${new_temp}=    Evaluate    ${CURRENT_TARGET} - 1
        Set Suite Variable    ${CURRENT_TARGET}    ${new_temp}
        Set Suite Variable    ${IS_TURBO}    ${False}
    END
    
    # Clamp Target Temp (16 to 30)
    IF    ${CURRENT_TARGET} > 30    Set Suite Variable    ${CURRENT_TARGET}    30
    IF    ${CURRENT_TARGET} < 16    Set Suite Variable    ${CURRENT_TARGET}    16
    
    _Update System State

_Update System State
    [Documentation]    Internal helper to calculate Fan/LCD state based on Temps.
    
    # Emergency Check
    IF    ${CURRENT_ROOM_TEMP} > ${EMERGENCY_TEMP_LIMIT}
        Set Suite Variable    ${IS_EMERGENCY}    ${True}
        Set Suite Variable    ${FAN_SPEED}       0
        Set Suite Variable    ${FAN_DIR}         OFF
        RETURN
    END

    ${diff}=    Evaluate    ${CURRENT_ROOM_TEMP} - ${CURRENT_TARGET}
    
    IF    ${IS_TURBO}
        Set Suite Variable    ${FAN_SPEED}    255
        Set Suite Variable    ${FAN_DIR}      FORWARD
        Set Suite Variable    ${MODE_STR}     TURB
    ELSE IF    ${diff} >= 1
        # Cooling Logic
        Set Suite Variable    ${FAN_DIR}      FORWARD
        Set Suite Variable    ${MODE_STR}     COOL
        IF    ${diff} >= 5
             Set Suite Variable    ${FAN_SPEED}    255
        ELSE IF    ${diff} >= 3
             Set Suite Variable    ${FAN_SPEED}    170
        ELSE
             Set Suite Variable    ${FAN_SPEED}    100
        END
    ELSE IF    ${diff} <= -1
        # Heating Logic
        Set Suite Variable    ${FAN_DIR}      REVERSE
        Set Suite Variable    ${MODE_STR}     HEAT
        ${abs_diff}=    Evaluate    abs(${diff})
        IF    ${abs_diff} >= 5
             Set Suite Variable    ${FAN_SPEED}    255
        ELSE IF    ${abs_diff} >= 3
             Set Suite Variable    ${FAN_SPEED}    170
        ELSE
             Set Suite Variable    ${FAN_SPEED}    100
        END
    ELSE
        # Idle Logic
        Set Suite Variable    ${FAN_DIR}      OFF
        Set Suite Variable    ${MODE_STR}     IDLE
        Set Suite Variable    ${FAN_SPEED}    0
    END

# --- Verification Keywords ---

The LCD Line 0 Should Display
    [Arguments]    ${expected_text}
    [Documentation]    Verifies Line 0 content.
    IF    ${IS_EMERGENCY}
         Log To Console    MOCK-VERIFY: Checking LCD Line 0 displays '!! OVERHEAT !! '
         # FIX: Removed quotes around ${expected_text} because it already has them
         Should Be Equal As Strings    "!! OVERHEAT !! "    ${expected_text}
    ELSE
         ${actual_text}=    Set Variable    "Set:${CURRENT_TARGET} C [${MODE_STR}]"
         Log To Console    MOCK-VERIFY: Checking LCD Line 0 displays '${actual_text}'
         # FIX: Removed quotes around ${expected_text} because it already has them
         Should Be Equal As Strings    ${actual_text}    ${expected_text}
    END

The LCD Line 1 Should Display
    [Arguments]    ${expected_text}
    [Documentation]    Verifies Line 1 content.
    IF    ${IS_EMERGENCY}
         ${actual_text}=    Set Variable    "Temp:${CURRENT_ROOM_TEMP} C SHUTDWN"
         Log To Console    MOCK-VERIFY: Checking LCD Line 1 displays '${actual_text}'
         # FIX: Removed quotes around ${expected_text}
         Should Be Equal As Strings    ${actual_text}    ${expected_text}
    ELSE
         ${percent}=    Evaluate    int((${FAN_SPEED} * 100) / 255)
         ${actual_text}=    Set Variable    "Rm :${CURRENT_ROOM_TEMP} C F:${percent}%"
         Log To Console    MOCK-VERIFY: Checking LCD Line 1 displays '${actual_text}'
         # FIX: Removed quotes around ${expected_text}
         Should Be Equal As Strings    ${actual_text}    ${expected_text}
    END

The Fan State Should Be
    [Arguments]    ${expected_speed}    ${expected_dir}
    [Documentation]    Verifies Motor PWM and Direction.
    Log To Console    MOCK-VERIFY: Checking Fan Speed=${FAN_SPEED} (Approx ${expected_speed}) and Dir=${FAN_DIR}
    Should Be Equal As Integers    ${FAN_SPEED}    ${expected_speed}
    Should Be Equal As Strings     ${FAN_DIR}      ${expected_dir}

*** Test Cases ***

TC-01: System Power On (Idle State)
    [Documentation]    Verifies defaults: Target 24C, Room 24C, Idle.
    [Tags]    Sanity    Boot
    Given Power On System
    And Set Room Temperature Sensor    24
    Then The LCD Line 0 Should Display    "Set:24 C [IDLE]"
    And The LCD Line 1 Should Display     "Rm :24 C F:0%"
    And The Fan State Should Be           0    OFF

TC-02: Cooling Logic (Moderate Heat)
    [Documentation]    Room is 27C, Target 24C (Diff +3). Fan should be Med Speed Forward.
    [Tags]    Functional    Cooling
    Given Power On System
    When Set Room Temperature Sensor    27
    Then The LCD Line 0 Should Display    "Set:24 C [COOL]"
    And The LCD Line 1 Should Display     "Rm :27 C F:66%"
    And The Fan State Should Be           170    FORWARD

TC-03: Cooling Logic (Max Heat)
    [Documentation]    Room is 30C, Target 24C (Diff +6). Fan should be Max Speed Forward.
    [Tags]    Functional    Cooling
    Given Power On System
    When Set Room Temperature Sensor    30
    Then The LCD Line 0 Should Display    "Set:24 C [COOL]"
    And The Fan State Should Be           255    FORWARD

TC-04: Heating Logic (Cold Room)
    [Documentation]    Room is 20C, Target 24C (Diff -4). Fan should be Med Speed Reverse.
    [Tags]    Functional    Heating
    Given Power On System
    When Set Room Temperature Sensor    20
    Then The LCD Line 0 Should Display    "Set:24 C [HEAT]"
    And The Fan State Should Be           170    REVERSE

TC-05: Turbo Mode Activation
    [Documentation]    Pressing KEY_RIGHT sets Target to 16C and Fan to Max.
    [Tags]    Functional    Turbo
    Given Power On System
    When Press Keypad Button    KEY_RIGHT
    Then The LCD Line 0 Should Display    "Set:16 C [TURB]"
    And The Fan State Should Be           255    FORWARD
    
    # Deactivate Turbo via Up Key
    When Press Keypad Button    KEY_UP
    Then The LCD Line 0 Should Display    "Set:17 C [COOL]"

TC-06: Adjust Target Temperature
    [Documentation]    Verify Up/Down buttons change the setpoint.
    [Tags]    Functional    Keypad
    Given Power On System
    When Press Keypad Button    KEY_UP
    Then The LCD Line 0 Should Display    "Set:25 C [HEAT]"
    
    When Press Keypad Button    KEY_DOWN
    When Press Keypad Button    KEY_DOWN
    Then The LCD Line 0 Should Display    "Set:23 C [COOL]"

TC-07: Emergency Overheat Logic
    [Documentation]    If Temp > 50C, System must Shutdown.
    [Tags]    Safety    Emergency
    Given Power On System
    When Set Room Temperature Sensor    55
    Then The LCD Line 0 Should Display    "!! OVERHEAT !! "
    And The LCD Line 1 Should Display     "Temp:55 C SHUTDWN"
    And The Fan State Should Be           0    OFF