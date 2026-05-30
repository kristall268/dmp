package org.dmp;

// public class HardwareService {
// public record HardwareSnapshot(String os_name, String os_version, String
// os_arch) {
// }

// // Метод сервиса
// public static HardwareSnapshot getOsSnapshot() {
// String os_name = System.getProperty("os.name");
// String os_version = System.getProperty("os.version");
// String os_arch = System.getProperty("os.arch");

// return new HardwareSnapshot(os_name, os_version, os_arch);
// }
// }
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class HardwareService {
    private static final short IPC_MAGIC = (short) 0x444D;
    private static final short CMD_LIST_DEVICES = 0x01;

    public record HardwareSnapshot(String Id, String Name, String DriverVersion)
}
