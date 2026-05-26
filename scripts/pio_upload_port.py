"""Pick CP210x USB-UART for upload/monitor unless upload_port is set explicitly."""
Import("env")


def _configured_port():
    try:
        port = env.GetProjectOption("upload_port")
    except Exception:
        return None
    if port and str(port).strip().lower() not in ("auto", ""):
        return str(port).strip()
    return None


def _find_cp210x():
    try:
        from serial.tools.list_ports import comports
    except ImportError:
        return None
    for p in comports():
        hwid = (p.hwid or "").upper()
        desc = (p.description or "").upper()
        if "10C4:EA60" in hwid or "CP210" in desc or "SILICON LABS" in desc:
            return p.device
    return None


configured = _configured_port()
if configured:
    env.Replace(UPLOAD_PORT=configured, MONITOR_PORT=configured)
    print("Using upload port from config: %s" % configured)
else:
    found = _find_cp210x()
    if found:
        env.Replace(UPLOAD_PORT=found, MONITOR_PORT=found)
        print("Auto-detected CP210x upload port: %s" % found)
    else:
        print(
            "WARNING: No CP210x serial port found. "
            "Copy platformio_local.ini.example to platformio_local.ini and set upload_port."
        )
