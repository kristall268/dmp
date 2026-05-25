package org.dmp;

public class HardwareService {
    public record HardwareSnapshot(String os_name, String os_version, String os_arch) {
    }

    // Метод сервиса
    public static HardwareSnapshot getOsSnapshot() {
        String os_name = System.getProperty("os.name");
        String os_version = System.getProperty("os.version");
        String os_arch = System.getProperty("os.arch");

        return new HardwareSnapshot(os_name, os_version, os_arch);
    }
}
