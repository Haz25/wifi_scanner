## Build

```
mkdir -p ros2_ws/src
cd ros2_ws/src
git clone https://github.com/Haz25/wifi_scanner.git
cd ..
colcon build --symlink-install
source ~/ros2_ws/install/setup.bash
```

## Run

```
ros2 run wifi_scanner wifi_scanner_node
```