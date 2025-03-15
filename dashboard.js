// // Sample data mapping DISCOMs to their respective meter lists
// // Wait for DOM to fully load before executing
// document.addEventListener('DOMContentLoaded', function() {
//   // Sample data mapping DISCOMs to their respective meter lists
//   const discomData = {
//     discom1: ['Meter 1A', 'Meter 1B', 'Meter 1C'],
//     discom2: ['Meter 2A', 'Meter 2B'],
//     discom3: ['Meter 3A', 'Meter 3B', 'Meter 3C', 'Meter 3D']
//   };

//   const discomList = document.getElementById('discomList');
//   const meterList = document.getElementById('meterList');
//   const selectedDiscom = document.getElementById('selectedDiscom');
//   const discomSearch = document.getElementById('discomSearch');
//   const meterSearch = document.getElementById('meterSearch');

//   // Function to update the meter list based on selected DISCOM
//   function updateMeterList(discomKey) {
//     // Clear existing meter list
//     meterList.innerHTML = '';
    
//     // Update displayed DISCOM
//     selectedDiscom.textContent = discomKey.toUpperCase();
    
//     // Get meter array for the selected DISCOM
//     const meters = discomData[discomKey] || [];
    
//     // Populate the meter list dynamically
//     meters.forEach(meter => {
//       const li = document.createElement('li');
//       li.textContent = meter;
//       li.addEventListener('click', function() {
//         // When a meter is clicked, update the meter info cards
//         document.getElementById('meterID').textContent = meter;
//         document.getElementById('powerConsumption').textContent = Math.floor(Math.random() * 1000) + ' kWh';
//         document.getElementById('timestamp').textContent = new Date().toLocaleString();
//         document.getElementById('billAmount').textContent = '$' + Math.floor(Math.random() * 500);
//       });
//       meterList.appendChild(li);
//     });
//   }

//   // Initialize event listeners for DISCOM list items
//   function initDiscomListeners() {
//     document.querySelectorAll('.discom-list li').forEach(item => {
//       item.addEventListener('click', function() {
//         // Remove active class from all DISCOM list items
//         document.querySelectorAll('.discom-list li').forEach(li => li.classList.remove('active'));
        
//         // Add active class to the clicked item
//         this.classList.add('active');
        
//         // Update meter list based on the clicked DISCOM
//         const discomKey = this.getAttribute('data-discom');
//         updateMeterList(discomKey);
//       });
//     });
//   }

//   // Filter DISCOM list items as user types in the DISCOM search box
//   discomSearch.addEventListener('input', function() {
//     const query = this.value.toLowerCase();
//     document.querySelectorAll('.discom-list li').forEach(li => {
//       li.style.display = li.textContent.toLowerCase().includes(query) ? '' : 'none';
//     });
//   });

//   // Filter meter list items as user types in the meter search box
//   meterSearch.addEventListener('input', function() {
//     const query = this.value.toLowerCase();
//     document.querySelectorAll('.meter-list li').forEach(li => {
//       li.style.display = li.textContent.toLowerCase().includes(query) ? '' : 'none';
//     });
//   });

//   // Initialize the listeners
//   initDiscomListeners();
// });


// Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyC3qC9a0opEjr_ThRv56Dp86AoUCtP0n6A",
  authDomain: "smartmeter-699c3.firebaseapp.com",
  databaseURL: "https://smartmeter-699c3-default-rtdb.firebaseio.com",
  projectId: "smartmeter-699c3",
  storageBucket: "smartmeter-699c3.firebasestorage.app",
  messagingSenderId: "34889062792",
  appId: "1:34889062792:web:18eea931e1ff63a0c6023d"
};

// Wait for DOM to be fully loaded
document.addEventListener('DOMContentLoaded', function() {
  console.log("DOM loaded, initializing Firebase...");
  
  // Initialize Firebase
  firebase.initializeApp(firebaseConfig);
  const database = firebase.database();
  
  // Elements for DOM manipulation
  const discomSearch = document.getElementById('discomSearch');
  const discomList = document.getElementById('discomList');
  const meterSearch = document.getElementById('meterSearch');
  const meterList = document.getElementById('meterList');
  const selectedDiscom = document.getElementById('selectedDiscom');
  const meterID = document.getElementById('meterID');
  const powerConsumption = document.getElementById('powerConsumption');
  const timestamp = document.getElementById('timestamp');
  const billAmount = document.getElementById('billAmount');
  const voltage = document.getElementById('voltage');
  const current = document.getElementById('current');
  
  // Store discom and meter data
  let discomData = {};
  let meterData = {};
  let currentDiscom = null;
  
  // Fetch initial data from Firebase
  function fetchInitialData() {
    console.log("Fetching data from Firebase...");
    // Reference to smartMeterData node
    const smartMeterRef = database.ref('smartMeterData');
    
    smartMeterRef.once('value')
      .then((snapshot) => {
        console.log("Firebase data received");
        const data = snapshot.val();
        console.log("Raw data:", data);
        
        if (!data) {
          console.log("No data found in Firebase");
          return;
        }
        
        // Process the data to organize by discomID
        processFirebaseData(data);
        
        // Populate DISCOM list
        populateDiscomList();
        
        // Set up listeners for real-time updates
        setupRealTimeListeners();
      })
      .catch(error => {
        console.error("Error fetching data:", error);
        alert("Failed to fetch data from Firebase: " + error.message);
      });
  }
  
  // Process and organize data from Firebase
  function processFirebaseData(data) {
    console.log("Processing Firebase data...");
    discomData = {};
    
    // Iterate through all entries to organize by discomID
    Object.keys(data).forEach(key => {
      const entry = data[key];
      console.log("Processing entry:", entry);
      
      // Skip if discomID is undefined
      if (!entry.discomID) {
        console.log("Skipping entry without discomID:", entry);
        return;
      }
      
      const discomID = entry.discomID;
      
      if (!discomData[discomID]) {
        discomData[discomID] = {
          meters: {},
          totalPowerDistributed: 0
        };
      }
      
      // If this is meter data with records
      if (entry.records && entry.meterID) {
        const meterID = entry.meterID;
        
        if (!discomData[discomID].meters[meterID]) {
          discomData[discomID].meters[meterID] = {
            latestPower: 0,
            latestVoltage: 0,
            latestCurrent: 0,
            latestTimestamp: 0,
            totalConsumption: 0,
            records: []
          };
        }
        
        // Process records
        entry.records.forEach(record => {
          // Add to meter's records
          discomData[discomID].meters[meterID].records.push(record);
          
          // Update latest values if this record is newer
          if (record.timestamp > discomData[discomID].meters[meterID].latestTimestamp) {
            discomData[discomID].meters[meterID].latestTimestamp = record.timestamp;
            // Convert values: in records they're scaled by 10
            discomData[discomID].meters[meterID].latestPower = record.power;
            discomData[discomID].meters[meterID].latestVoltage = record.voltage;
            discomData[discomID].meters[meterID].latestCurrent = record.current;
          }
          
          // Add to total consumption (simplified calculation)
          discomData[discomID].meters[meterID].totalConsumption += record.power / 10;
          
          // Add to DISCOM's total distributed power
          discomData[discomID].totalPowerDistributed += record.power / 10;
        });
      }
    });
    
    console.log("Processed data structure:", discomData);
  }
  
  // Populate the DISCOM list in the sidebar
  function populateDiscomList() {
    console.log("Populating DISCOM list...");
    discomList.innerHTML = '';
    
    Object.keys(discomData).forEach(discomID => {
      const li = document.createElement('li');
      li.textContent = `DISCOM ${discomID}`;
      li.setAttribute('data-discom', discomID);
      li.addEventListener('click', () => selectDiscom(discomID));
      discomList.appendChild(li);
    });
    
    if (Object.keys(discomData).length === 0) {
      const li = document.createElement('li');
      li.textContent = "No DISCOMs found";
      discomList.appendChild(li);
    }
  }
  
  // Select a DISCOM and show its meters
  function selectDiscom(discomID) {
    console.log("Selected DISCOM:", discomID);
    currentDiscom = discomID;
    
    // Update UI to show selected DISCOM
    selectedDiscom.textContent = `DISCOM ${discomID}`;
    document.querySelectorAll('#discomList li').forEach(li => {
      li.classList.remove('selected');
      if (li.getAttribute('data-discom') === discomID) {
        li.classList.add('selected');
      }
    });
    
    // Update power distributed display
    const powerDistributed = discomData[discomID].totalPowerDistributed.toFixed(2);
    document.querySelector('.top-info .info-card:nth-child(2) p').textContent = `${powerDistributed} kWh`;
    
    // Populate meter list for this DISCOM
    populateMeterList(discomID);
    
    // Clear meter details
    clearMeterDetails();
  }
  
  // Populate meter list for selected DISCOM
  function populateMeterList(discomID) {
    console.log("Populating meter list for DISCOM:", discomID);
    meterList.innerHTML = '';
    
    const meters = discomData[discomID].meters;
    Object.keys(meters).forEach(meterID => {
      const li = document.createElement('li');
      li.textContent = `Meter ${meterID}`;
      li.setAttribute('data-meter', meterID);
      li.addEventListener('click', () => selectMeter(meterID));
      meterList.appendChild(li);
    });
    
    if (Object.keys(meters).length === 0) {
      const li = document.createElement('li');
      li.textContent = "No meters found";
      meterList.appendChild(li);
    }
  }
  
  // Select a meter and show its details
  function selectMeter(meterID) {
    console.log("Selected meter:", meterID);
    if (!currentDiscom) return;
    
    const meter = discomData[currentDiscom].meters[meterID];
    
    // Update UI to show selected meter
    document.querySelectorAll('#meterList li').forEach(li => {
      li.classList.remove('selected');
      if (li.getAttribute('data-meter') === meterID) {
        li.classList.add('selected');
      }
    });
    
    // Update meter details
    document.getElementById('meterID').textContent = meterID;
    document.getElementById('powerConsumption').textContent = `${meter.latestPower.toFixed(2)} W`;
    
    // Update voltage and current
    document.getElementById('voltage').textContent = `${meter.latestVoltage.toFixed(1)} V`;
    document.getElementById('current').textContent = `${meter.latestCurrent.toFixed(2)} A`;
    
    // Format timestamp for display
    const date = new Date(meter.latestTimestamp * 1000);
    document.getElementById('timestamp').textContent = date.toLocaleString();
    
    // Calculate bill amount (simplified)
    const rate = 8.0; // Example rate per kWh (in rupees)
    const bill = meter.totalConsumption * rate / 1000; // Convert W to kW
    document.getElementById('billAmount').textContent = `₹${bill.toFixed(2)}`;
  }
  
  // Clear meter details when no meter is selected
  function clearMeterDetails() {
    document.getElementById('meterID').textContent = '--';
    document.getElementById('powerConsumption').textContent = '-- W';
    document.getElementById('timestamp').textContent = '--';
    document.getElementById('billAmount').textContent = '--';
    document.getElementById('voltage').textContent = '-- V';
    document.getElementById('current').textContent = '-- A';
  }
  
  // Set up real-time listeners for data updates
  function setupRealTimeListeners() {
    console.log("Setting up real-time listeners...");
    const smartMeterRef = database.ref('smartMeterData');
    
    smartMeterRef.on('child_added', (snapshot) => {
      console.log("New data added:", snapshot.key);
      const newData = {};
      newData[snapshot.key] = snapshot.val();
      processFirebaseData(newData);
      
      // Refresh UI if needed
      if (currentDiscom) {
        populateDiscomList();
        populateMeterList(currentDiscom);
      }
    });
    
    smartMeterRef.on('child_changed', (snapshot) => {
      console.log("Data changed:", snapshot.key);
      const changedData = {};
      changedData[snapshot.key] = snapshot.val();
      processFirebaseData(changedData);
      
      // Refresh UI if needed
      if (currentDiscom) {
        populateDiscomList();
        populateMeterList(currentDiscom);
      }
    });
  }
  
  // Search functionality for DISCOMs
  discomSearch.addEventListener('input', (e) => {
    const searchTerm = e.target.value.toLowerCase();
    
    document.querySelectorAll('#discomList li').forEach(li => {
      const discomText = li.textContent.toLowerCase();
      if (discomText.includes(searchTerm)) {
        li.style.display = 'block';
      } else {
        li.style.display = 'none';
      }
    });
  });
  
  // Search functionality for meters
  meterSearch.addEventListener('input', (e) => {
    const searchTerm = e.target.value.toLowerCase();
    
    document.querySelectorAll('#meterList li').forEach(li => {
      const meterText = li.textContent.toLowerCase();
      if (meterText.includes(searchTerm)) {
        li.style.display = 'block';
      } else {
        li.style.display = 'none';
      }
    });
  });
  
  // Initialize the dashboard
  fetchInitialData();
  
  // Add a simple test to verify Firebase connection
  console.log("Testing direct Firebase access...");
  database.ref().child('.info/connected').on('value', (snap) => {
    if (snap.val() === true) {
      console.log("Connected to Firebase!");
    } else {
      console.log("Disconnected from Firebase!");
    }
  });
});