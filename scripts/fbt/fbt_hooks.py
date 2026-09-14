def PreConfigureFwEnvionment(env):
    # Nuerolynx retains the complete Momentum-compatible application and
    # settings APIs while reporting its own firmware identity to the device.
    env.AppendUnique(CPPDEFINES=["FW_ORIGIN_Momentum"])
