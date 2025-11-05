#include <rclcpp/rclcpp.hpp>
#include "wifi_scanner/msg/wifi_scan.hpp"
#include "wifi_scanner/msg/wifi_top.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using wifi_scanner::msg::WifiScan;
using wifi_scanner::msg::WifiTop;

struct AP {
  std::string ssid;
  std::string bssid;
  int freq;
  int power;
};

static bool isLAA(const std::string& bssid) {
  unsigned int b0, b1, b2, b3, b4, b5;
  sscanf(bssid.c_str(), "%02x%02x%02x%02x%02x%02x", &b0,&b1,&b2,&b3,&b4,&b5);
  return (b0 & 0x02) != 0;
}

static std::vector<AP> run_scan() {
  std::vector<AP> v;

  const char* cmd = "/usr/bin/env LC_ALL=C /usr/bin/nmcli --colors no -t -f SSID,BSSID,FREQ,SIGNAL dev wifi 2>/dev/null";
  std::string out;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  char buf[1024];
  while (fgets(buf, sizeof(buf), pipe.get())) 
    out.append(buf);

  std::istringstream iss(out);
  std::string line;
  while (std::getline(iss, line)) {
    for (size_t pos = 0; (pos = line.find("\\:", pos)) != std::string::npos; )
      line.replace(pos, 2, "");

    int sep1 = -1, sep2 = -1, sep3 = -1;
    for (int i=0; i<(int)line.size(); ++i) {
      if (line[i] != ':') continue;

      if (sep1 < 0) sep1 = i;
      else if (sep2 < 0) sep2 = i;
      else if (sep3 < 0) sep3 = i;
    }

    AP ap;
    ap.ssid = line.substr(0, sep1);
    ap.bssid = line.substr(sep1+1, sep2-sep1-1);
    ap.freq = std::stoi(line.substr(sep2+1, sep3-sep2-1));
    ap.power = std::stoi(line.substr(sep3+1)) / 5 * 5;
    v.push_back(ap);
  }

  return v;
}

class WifiScannerNode : public rclcpp::Node {
public:
  WifiScannerNode() : Node("wifi_scanner_node") {
    pub_scan_ = create_publisher<WifiScan>("/wifi/scan", 10);
    pub_top_  = create_publisher<WifiTop> ("/wifi/top_ssid", 10);
    timer_ = create_wall_timer(std::chrono::duration<double>(1.0),
              [this]{ tick(); });
  }

private:
  void tick() {
    auto aps = run_scan();

    WifiScan s;
    s.stamp = rclcpp::Clock().now();
    for (auto& a : aps) {
      s.ssid.push_back(a.ssid);
      s.bssid.push_back(a.bssid);
      s.freq.push_back(a.freq);
      s.power.push_back(a.power);
    }
    pub_scan_->publish(s);

    sort(aps.begin(), aps.end(), [](AP &ap1, AP &ap2){
        return ap1.power != ap2.power? ap1.power > ap2.power: ap1.freq != ap2.freq? ap1.freq > ap2.freq: ap1.ssid < ap2.ssid;
    });

    WifiTop t; t.stamp = s.stamp;
    for (auto& a : aps) {
      if (!isLAA(a.bssid)) {
        t.ssid  = a.ssid;
        t.bssid = a.bssid;
        t.freq  = a.freq;
        t.power = a.power;
        pub_top_->publish(t);
        return;
      }
    }
  }

  rclcpp::Publisher<WifiScan>::SharedPtr pub_scan_;
  rclcpp::Publisher<WifiTop>::SharedPtr pub_top_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<WifiScannerNode>());
  rclcpp::shutdown();
  return 0;
}