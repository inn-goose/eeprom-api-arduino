import sys
import glob
import serial


def serial_ports():
    # supports MacOS only, sorry
    if sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/cu.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException) as ex:
            print(ex)
    return result


if __name__ == '__main__':
    print(serial_ports())
