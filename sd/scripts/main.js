/* Wifi Auth Modes as defined by the firmware */
const WIFI_AUTH_MODES = {
  'WIFI_AUTH_OPEN': '0',             /**< authenticate mode : open */
  'WIFI_AUTH_WEP': '1',              /**< authenticate mode : WEP */
  'WIFI_AUTH_WPA_PSK': '2',          /**< authenticate mode : WPA_PSK */
  'WIFI_AUTH_WPA2_PSK': '3',         /**< authenticate mode : WPA2_PSK */
  'WIFI_AUTH_WPA_WPA2_PSK': '4',     /**< authenticate mode : WPA_WPA2_PSK */
  'WIFI_AUTH_WPA2_ENTERPRISE': '5',  /**< authenticate mode : WPA2_ENTERPRISE */
  'WIFI_AUTH_MAX': '6'
};

const getAuthModeText = (mode) => {
  return Object.entries(WIFI_AUTH_MODES).filter(([key, value]) => (mode === value)).pop().slice(0, 1).pop();
};

/*
SIGNAL STRENGTH	EXPECTED QUALITY	REQUIRED FOR
-30 dBm	Maximum signal strength, you are probably standing right next to the access point.	
-50 dBm	Anything down to this level can be considered excellent signal strength.	
-60 dBm	Good, reliable signal strength.	
-67 dBm	Reliable signal strength.	The minimum for any service depending on a reliable connection and signal strength, such as voice over Wi-Fi and non-HD video streaming.
-70 dBm	Not a strong signal.	Light browsing and email.
-80 dBm	Unreliable signal strength, will not suffice for most services.	Connecting to the network.
-90 dBm	The chances of even connecting are very low at this level.	
*/

/* Wifi Signal Strength Ranges */
const SIGNAL_STRENGTH_TEXT = [
  [[0, -30], 'Maximum'],
  [[-31, -50], 'Excellent'],
  [[-51, -60], 'Good'],
  [[-61, -67], 'Reliable'],
  [[-68, -70], 'Not Strong'],
  [[-71, -80], 'Unreliable'],
  [[-81, -90], 'Unstable'],
];

const getStrengthText = (signalStrength) => {
  /* find if signal is between ranges */
  return SIGNAL_STRENGTH_TEXT.filter(e => (signalStrength <= e[0][0] && signalStrength >= e[0][1]))
    .pop()
    .slice(1)
    .pop();
};

/* Very basic EventEmitter class */
class EventEmitter {
  constructor() {
    this.events = new Map();
  }
  on(eventname, callback) {
    if(this.events.has(eventname)) {
      const events = this.events.get(eventname);
      events.push(callback);
      return this.events.set(eventname, events);
    }
    return this.events.set(eventname, [callback]);
  }
  off(eventname, callback) {
    if(this.events.has(eventname)) {
      const events = this.events.get(eventname).filter(clbk => (clbk !== callback));
      return this.events.set(eventname, events);
    }
    return undefined;
  }
  emit(eventname, ...args) {
    if (this.events.has(eventname)) {
      this.events.get(eventname).forEach(function(callback) {
        callback.apply(this, args);
      });
    }
  }
}

const autoTime = (checkbox) =>{
  const { checked } = checkbox;
  const timeServerInput = document.getElementById('time-server');
  const customDateTimefieldset = document.getElementById('custom-date-time');
  if(checked) {
    timeServerInput.setAttribute('required', '');
    customDateTimefieldset.setAttribute('disabled', '');
  } else {
    timeServerInput.setAttribute('disabled', '');
    timeServerInput.removeAttribute('required');
    customDateTimefieldset.removeAttribute('disabled');
  }
};

const showSpinner = (show) => {
  const formOverlay = document.getElementById('form-overlay');
  formOverlay.style.display = show ? 'block' : 'none';
};

const showPassword = (checkbox) => {
  const { checked } = checkbox;
  const password = document.getElementById('password');
  if (checked) {
    password.type = 'text';
  } else {
    password.type = 'password';
  }
}
const checkFormValidity = () => {
  const form = document.querySelector('form');
  return form.checkValidity();
};

const invalidHanlder = (event) => {
  const { target } = event;
  target.classList.add('field-alert');
};

const changeHanlder = (event) => {
  const { target } = event;
  if(target.classList.contains('field-alert')) {
    target.classList.remove('field-alert');
  }
};

const inputHanlder = (event) => {
  const { target } = event;
  if(target.classList.contains('field-alert')) {
    target.classList.remove('field-alert');
  }
};

const wifiSelect = (network) => {
  console.log('network', network);
  const wifiConfigurationFieldset = document.querySelector('fieldset#wifi-configuration');
  const wifiPropsElement = wifiConfigurationFieldset.querySelector('.wifi-props');
  const wifiSignalStrengthElement = wifiConfigurationFieldset.querySelector('.wifi-signal-strength');
  const wifiSignalEncryptionType = wifiConfigurationFieldset.querySelector('.wifi-signal-encryption');

  wifiPropsElement.style.display = 'block';
  wifiSignalStrengthElement.textContent = getStrengthText(network.rssi);
  wifiSignalEncryptionType.textContent = getAuthModeText(network.encryptionType);
  
  console.log(network.encryptionType, WIFI_AUTH_MODES['WIFI_AUTH_OPEN']);
  /* If wifi requires credentials then show the password input */
  if(network.encryptionType !== WIFI_AUTH_MODES['WIFI_AUTH_OPEN']) {
    const password = document.getElementById('password');
    const passwordField = document.querySelector('#password-field');
    password.setAttribute('required', '');
    passwordField.style.display = 'block';
  } else {
    const password = document.getElementById('password');
    const passwordField = document.querySelector('#password-field');
    password.removeAttribute('required');
    passwordField.style.display = 'none';
  }
}

const setupInputValidations = () => {
  const inputs = Array.from(document.querySelectorAll('input')).filter(input => input.getAttribute('type') !== 'button');
  const selects = Array.from(document.querySelectorAll('select'));

  [...inputs, ...selects].forEach(element => {
    element.addEventListener('invalid', invalidHanlder);
    element.addEventListener('change', changeHanlder);
    element.addEventListener('input', inputHanlder);
  });
}


const ssidChange = (networks) => {
  return (event) => {
    const { selectedIndex } = event.target;
    wifiSelect(networks[selectedIndex]);
  }
}

const setWifiOptions = (networks) => {
  /* get dropdown element */
  const select = document.getElementById('ssid');
  /* get options and remove them */
  select.querySelectorAll('option').forEach(option => option.remove());
  /* add new options */
  networks.forEach((option, index) => {
    select.add(new Option(option.ssid, option.index));
  });
  select.onchange = ssidChange(networks);
}

const mapNetworks = (networks) => {
  return Array.from(networks.querySelector('networks').children)
    .map(network => {
      const ssid = network.querySelector('ssid').textContent;
      const encryptionType = network.querySelector('encryption-type').textContent;
      const rssi = network.querySelector('rssi').textContent;
      return {
        ssid,
        encryptionType,
        rssi,
      };
    });
};

const refreshWifi = () => {
  showSpinner(true);
  fetchWifiConfig()
    .then(data => mapNetworks(data))
    .then(networks => {
      setWifiOptions(networks);
      showSpinner(false);
    })
    .catch(error => {
      console.error(error);
      showSpinner(false);
    });
};

const refreshRtc = (event) => {
  return Promise.resolve({
    rtc: Math.round(new Date().getTime() / 1000),
  });
  // return fetch('/rtc')
  return fetch('/rtc')
  .then(response => {
    if (response.status !== 200) {
      console.error('Looks like there was a problem. Status Code: ' + response.status);
      throw new Error(response.status);
    }
    return response.json();
  });
};

const wifis = `
<wifi-config>
  <connection-ssid>192.168.0.1</connection-ssid>
  <ssid>caca</ssid>
  <ip>0.0.0.0</ip>
  <networks>
    <network>
      <ssid>My Wifi</ssid>
      <encryption-type>4</encryption-type>
      <rssi>-54</rssi>
    </network>
    <network>
      <ssid>My Open Wifi</ssid>
      <encryption-type>0</encryption-type>
      <rssi>-88</rssi>
    </network>
    <network>
      <ssid>My Second Wifi</ssid>
      <encryption-type>4</encryption-type>
      <rssi>-77</rssi>
    </network>
  </networks>
</wifi-config>
`;

const fetchWifiConfig = () => {
  return new Promise(resolve => {
    setTimeout(() => {
      const document = new window.DOMParser().parseFromString(wifis, "text/xml");
      resolve(document);
    }, 1000);
  });
  return fetch('/wifinetworkconfig')
    .then(response => {
      if (response.status !== 200) {
        console.error('Looks like there was a problem. Status Code: ' + response.status);
        throw new Error(response.status);
      }
      return response;
    })
    .then(response => response.text())
    .then(string => (new window.DOMParser()).parseFromString(string, "text/xml"));
}

const rtcUpdate = () => {
  const event = new EventEmitter();
  setInterval(() => {
    refreshRtc().then(({ rtc }) => {
      const epoch = document.getElementById('epoch');
      epoch.value = rtc;
    });
  }, 1000);
}

const next = (event) => {
  const isFormValid = checkFormValidity();
  if(!isFormValid) {
    return;
  }
  console.log('next');
  const element = event.target;
  const currentFieldSet = element.closest('fieldset');
	const nextFieldSet = currentFieldSet.nextElementSibling;
  const progressbar = document.querySelector('#progressbar');
  const fieldsets = Array.prototype.slice.call(document.querySelectorAll('#wificonfig > fieldset'));
  const stepIndex = fieldsets.indexOf(nextFieldSet);
  const step = progressbar.getElementsByTagName('li').item(stepIndex);
  step.classList.add('active');
  currentFieldSet.style.display = 'none';
  nextFieldSet.style.display = 'block';
};

const previous = (event) => {
  const element = event.target;
  const currentFieldSet = element.closest('fieldset');
	const previousFieldSet = currentFieldSet.previousElementSibling;
  const progressbar = document.querySelector('#progressbar');
  const fieldsets = Array.prototype.slice.call(document.querySelectorAll('#wificonfig > fieldset'));
  const stepIndex = fieldsets.indexOf(currentFieldSet);
  const step = progressbar.getElementsByTagName('li').item(stepIndex);
  step.classList.remove('active');
  currentFieldSet.style.display = 'none';
  previousFieldSet.style.display = 'block';
};

const ready = (event) => {
  const nextButton = document.querySelectorAll('input[name="next"]');
  const prevButton = document.querySelectorAll('input[name="previous"]');
  /* Attach handler for the next buttons */
  Array.from(nextButton).forEach(input => {
    input.onclick = next;
  });
  /* Attach handler for the previous buttons */
  Array.from(prevButton).forEach(input => {
    input.onclick = previous;
  });

  setupInputValidations();
  refreshWifi();
  rtcUpdate();
};

/* main */
window.onload = ready;