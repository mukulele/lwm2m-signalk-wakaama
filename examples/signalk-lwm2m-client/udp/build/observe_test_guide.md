# Testing Server-Initiated Observations with 1NCE - PRACTICAL GUIDE

## 🎯 **Ready-to-Test Setup (Enhanced for Observation)**

Your client now has **enhanced observation debugging** and a **manual test trigger**!

### **Observable Resources Available**

1. **Device Object (3/0)** - ⭐ **BEST FOR TESTING**
   - `/3/0/9` - Battery Level (auto-changes every 5 seconds)
   - `/3/0/13` - Current Time (continuously updating)
   - `/3/0/16` - Binding Mode

2. **Location Object (6/0)** - GPS Data
   - `/6/0/0` - Latitude
   - `/6/0/1` - Longitude  
   - `/6/0/5` - Timestamp

3. **Test Object (1024/0)** - For manual testing
   - `/1024/0/1` - Test resource

---

## � **Step-by-Step Test Process**

### Step 1: Start Your Enhanced Client
```bash
cd /home/pi/wakaama/examples/signalk-lwm2m-client/udp/build
./signalk-lwm2m-client -b -h lwm2m.os.1nce.com -p 5683 -4 -k
```

**Wait for these confirmations:**
- `STATE_READY` 
- `[SIGNALK] LwM2M registration complete`
- `[SIGNALK] Bridge system ready`

### Step 2: Send OBSERVE Request from 1NCE

**Recommended Test Target:** `/3/0/9` (Battery Level)

From your 1NCE server interface:
```
Method: OBSERVE (GET with Observe=0 option)
Path: /3/0/9
```

**Expected Client Output:**
```
[OBSERVE] Resource /3/0/9 is being observed
```

### Step 3: Watch Automatic Notifications

Every 5 seconds you should see:
```
[OBSERVE] Sending notification for /3/0/9 - value changed
```

And network traffic showing notification packets to 1NCE server.

### Step 4: Manual Testing (NEW!)

In the client console, type:
```
testobs
```

**This will:**
- Force immediate battery level change
- Trigger current time notification  
- Show detailed debug output:
```
[OBSERVE] Forcing battery level change to test observations
[OBSERVE] Triggering notification for /3/0/9 (Battery Level)
[OBSERVE] Triggering notification for /3/0/13 (Current Time)
[OBSERVE] Test notifications sent - check for observe responses
```

---

## 📊 **What to Expect from 1NCE Server**

### Initial OBSERVE Request → Response:
1. **1NCE sends:** `GET /3/0/9` with `Observe: 0`
2. **Client responds:** `2.05 Content` with current battery value
3. **Client output:** `[OBSERVE] Resource /3/0/9 is being observed`

### Ongoing Notifications:
1. **Battery changes** (every 5 seconds automatically)
2. **Client sends:** `2.05 Content` notification to 1NCE
3. **Client output:** `[OBSERVE] Triggering notification for /3/0/9`

### Manual Testing:
1. **You type:** `testobs` in client console
2. **Client immediately sends** notifications to 1NCE
3. **Real-time validation** of observation system

---

## 🐛 **Debug Information You'll See**

### When Observation Starts:
```
[OBSERVE] Resource /3/0/9 is being observed
```

### When Values Change:
```
[OBSERVE] Triggering notification for /3/0/9 (Battery Level)
[OBSERVE] Triggering resource value change for /3/0/9
```

### Network Traffic:
```
Sending X bytes to [lwm2m.os.1nce.com]:5683
```

---

## 🎛️ **Testing Strategies**

### Strategy 1: Automatic Testing
- Observe `/3/0/9` from 1NCE
- Wait and watch automatic 5-second notifications
- Reliable and continuous

### Strategy 2: Manual Trigger Testing  
- Observe any resource from 1NCE
- Use `testobs` command for immediate testing
- Perfect for validation and demonstration

### Strategy 3: SignalK-Driven Testing
- Observe any resource while SignalK is running
- Real marine data will trigger additional notifications
- Tests real-world scenario

---

## ✅ **Success Indicators**

### From Client Side:
- [ ] `[OBSERVE] Resource /X/Y/Z is being observed`
- [ ] `[OBSERVE] Triggering notification for /X/Y/Z`  
- [ ] Regular network traffic to 1NCE server
- [ ] `testobs` command works and shows debug output

### From 1NCE Server Side:
- [ ] Initial observe response received
- [ ] Periodic notification packets arriving
- [ ] Notification content matches expected values
- [ ] Can cancel observations successfully

---

## 🎯 **Recommended Test Sequence**

1. **Start client** and wait for registration
2. **Observe `/3/0/9`** from 1NCE (most reliable)
3. **Watch automatic notifications** for 30 seconds
4. **Use `testobs`** to trigger immediate notifications
5. **Test multiple resources** like `/3/0/13`
6. **Cancel observation** from 1NCE to test cleanup

---

This setup gives you **maximum visibility** into the observation process and **multiple ways to test** both automatic and manual scenarios!
