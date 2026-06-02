#include <WiFi.h>
#include <HTTPClient.h>
#include <SdFat.h>
#include <mbedtls/sha1.h>

String my_server = "http://homeassistant.local:8099/esp32";

String generateKey()
{
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
        return "";

    char timestamp[16];

    strftime(
        timestamp,
        sizeof(timestamp),
        "%Y%m%d%H",
        &timeinfo);

    String base =
        String("CHANGE_THIS_SECRET") +
        timestamp;

    uint8_t shaResult[20];

    mbedtls_sha1(
        (const unsigned char *)base.c_str(),
        base.length(),
        shaResult);

    char hex[41];

    for (int i = 0; i < 20; i++)
        sprintf(hex + i * 2, "%02x", shaResult[i]);

    hex[40] = 0;
    // Serial.println(base);
    // Serial.println(String(hex));
    return String(hex);
}

bool pingServer()
{
    HTTPClient http;

    String url = my_server + "/cmd?cmd=ping";

    http.begin(url);

    http.addHeader(
        "X-ESP32-KEY",
        generateKey());

    int code = http.GET();

    if (code != 200)
    {
        http.end();
        log_e("Error pinging server :%d", code);
        return false;
    }

    String response = http.getString();

    http.end();
    return response.indexOf("pong") >= 0;
}

bool uploadFile(String filename)
{
    File32 file;

    if (!file.open(filename.c_str(), O_RDONLY))
        return false;

    HTTPClient http;

    // http.begin("http://192.168.1.100:8099/esp32/upload");
    http.begin(my_server + "/upload");

    http.addHeader("X-ESP32-KEY", generateKey());
    http.addHeader("X-FILENAME", filename);
    http.addHeader("Content-Type", "application/octet-stream");

    int code = http.sendRequest(
        "POST",
        &file,
        file.fileSize());
    // Serial.println("file size " + String(file.fileSize()));
    file.close();

    String response = http.getString();
    JsonDocument jsonResponce;
    deserializeJson(jsonResponce, response);
    http.end();

    Serial.println(code);
    Serial.println(response);

    return (code == 200 && jsonResponce["size"] > 0);
}

int get_filenames(bool _silent = false)
{

    num_of_files = 0;

    if (sd_ready)
    {
        if (!_silent)
        {
            Serial.println("listing SD files in /");
        }

        File32 root;
        root.open("/");
        File32 lstfile = root.openNextFile();

        while (lstfile)
        {
            if (num_of_files >= max_filenames)
                break;
            if (lstfile.isDirectory())
            {
                lstfile = root.openNextFile();
                continue;
            }
            char filenames_tmp[40];
            size_t filename_size = lstfile.getName(filenames_tmp, sizeof(filenames_tmp));
            String name(filenames_tmp);
            if (!name.endsWith(".csv"))
            {
                lstfile = root.openNextFile();
                continue;
            }
            if (!_silent)
            {
                if (!lstfile.attrib(0))
                {
                    Serial.println("clearing attributes failed");
                }
                Serial.print("FILE: ");
                Serial.print(name);
                Serial.print("  ");
                Serial.println(lstfile.size());
            }

            dir_filenames_array[num_of_files] = name;
            num_of_files++;
            lstfile = root.openNextFile();
        }
        lstfile.close();
        root.close();

        return num_of_files;
    }

    return 0;
}

void delete_all_trips()
{
    for (int i = get_filenames(); i > 0; i--)
    {
        if (sd_ready)
        {
            sdfile.open(dir_filenames_array[max(i - 1, 0)].c_str(), O_RDWR);
            if (!sdfile.remove())
            {
                log_e("SD file not deleted %d ", sdfile.getError(), dir_filenames_array[max(i - 1, 0)].c_str());
            }
            sdfile.close();
        }
        else
        {
            SPIFFS.remove(dir_filenames_array[max(i - 1, 0)]);
        }
    }
}

enum UploadFailReason
{
    UPLOAD_OK = 0,
    FAIL_NOT_CSV,
    FAIL_FILE_MISSING,
    FAIL_NO_WIFI,
    FAIL_SERVER_UNREACHABLE,
    FAIL_UPLOAD_ERROR
};

const char *uploadFailToString(UploadFailReason r)
{
    switch (r)
    {
    case UPLOAD_OK:
        return "OK";

    case FAIL_NOT_CSV:
        return "NOT_CSV";

    case FAIL_FILE_MISSING:
        return "FILE_MISSING";

    case FAIL_NO_WIFI:
        return "NO_WIFI";

    case FAIL_SERVER_UNREACHABLE:
        return "SERVER_UNREACHABLE";

    case FAIL_UPLOAD_ERROR:
        return "UPLOAD_ERROR";

    default:
        return "UNKNOWN";
    }
}

UploadFailReason lastUploadFail = UPLOAD_OK;

#define MAX_RETRIES 3
#define WIFI_TIMEOUT_MS 100

bool isCsvFile(String filename)
{
    return filename.endsWith(".csv");
}

bool isWifiOk()
{
    return WiFi.status() == WL_CONNECTED;
}

bool fileExists(File32 &sd, String path)
{
    // sd.open(path.c_str(), O_EXCL | O_RDONLY);
    bool res = !sd.open(path.c_str(), O_EXCL | O_RDONLY);
    sd.close();
    return res;
}

void uploadPendingFiles()
{
    File32 sd;
    lastUploadFail = UPLOAD_OK;

    if (!isWifiOk())
    {
        lastUploadFail = FAIL_NO_WIFI;
        Serial.println("WiFi not connected");
        return;
    }

    if (!pingServer())
    {
        lastUploadFail = FAIL_SERVER_UNREACHABLE;
        Serial.println("Server not reachable");
        return;
    }

    for (int i = 0; i < num_of_files; i++)
    {
        String filename = dir_filenames_array[i];

        if (!filename)
            continue;

        Serial.print("Processing: ");
        Serial.println(filename);

        // -------------------------
        // CSV check
        // -------------------------
        if (!isCsvFile(filename))
        {
            Serial.println("Skip (not CSV)");
            lastUploadFail = FAIL_NOT_CSV;
            continue;
        }

        // -------------------------
        // Exists check
        // -------------------------
        if (!fileExists(sd, filename))
        {
            Serial.println("File missing");
            lastUploadFail = FAIL_FILE_MISSING;
            continue;
        }

        // -------------------------
        // Retry upload
        // -------------------------
        bool success = false;

        for (int attempt = 1; attempt <= MAX_RETRIES; attempt++)
        {
            Serial.print("Upload attempt ");
            Serial.println(attempt);

            if (!isWifiOk())
            {
                lastUploadFail = FAIL_NO_WIFI;
                break;
            }

            if (!pingServer())
            {
                lastUploadFail = FAIL_SERVER_UNREACHABLE;
                break;
            }

            if (uploadFile(filename))
            {
                success = true;
                break;
            }

            delay(100); // simple backoff
        }

        // -------------------------
        // Final result
        // -------------------------
        if (success)
        {
            Serial.println("Upload OK, deleting file");

            if (!sd.open(filename.c_str(), O_WRONLY))
            {
                Serial.println("Delete failed (ignored)");
            }
            else
            {
                sd.remove();
            }
            sd.close();
            lastUploadFail = UPLOAD_OK;
        }
        else
        {
            Serial.println("Upload failed");
            lastUploadFail = FAIL_UPLOAD_ERROR;
        }
    }
}