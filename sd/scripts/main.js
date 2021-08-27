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

const showSpinner = (show) => {
  const formOverlay = document.getElementById('form-overlay');
  console.log(formOverlay);
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
  console.log('invalidInputHanlder', event);
  const { target } = event;
  target.classList.add('field-alert');
};

const changeHanlder = (event) => {
  console.log('changeInputHanlder', event);
  const { target } = event;
  if(target.classList.contains('field-alert')) {
    target.classList.remove('field-alert');
  }
};

const inputHanlder = (event) => {
  console.log('inputHanlder', event);
  const { target } = event;
  if(target.classList.contains('field-alert')) {
    target.classList.remove('field-alert');
  }
};

const setupInputValidations = () => {
  const inputs = Array.from(document.querySelectorAll('input')).filter(input => input.getAttribute('type') !== 'button');
  const selects = Array.from(document.querySelectorAll('select'));

  [...inputs, ...selects].forEach(element => {
    console.log('element', element)
    element.addEventListener('invalid', invalidHanlder);
    element.addEventListener('change', changeHanlder);
    element.addEventListener('input', inputHanlder);
  });
}

const setWifiOptions = (options) => {
  /* get dropdown element */
  const select = document.querySelector('#wifi-configuration #ssid');
  /* get options and remove them */
  select.querySelectorAll('option').forEach(option => option.remove());
  /* add new options */
  options.forEach((option, index) => {
    select.add(new Option(option.ssid, option.index));
  });
}

const mapNetworks = (wifiConfig) => {
  return Array.from(wifiConfig.querySelector('networks').children)
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
    .then(wifiConfig => {
      const networks = mapNetworks(wifiConfig);
      setWifiOptions(networks);
      showSpinner(false);
    })
    .catch(error => {
      console.error(error);
      showSpinner(false);
    });
};

const refreshRtc = (event) => {
  // return Promise.resolve({
  //   rtc: new Date().getTime(),
  // });
  // return fetch('/rtc')
  return fetch('https://gorest.co.in/public/v1/users')
  .then(response => {
    if (response.status !== 200) {
      console.log('Looks like there was a problem. Status Code: ' + response.status);
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
      <rssi>-77</rssi>
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
  // return new Promise(resolve => {
  //   setTimeout(() => {
  //     const document = new window.DOMParser().parseFromString(wifis, "text/xml");
  //     resolve(document);
  //   }, 1000);
  // });
  return fetch('/wifinetworkconfig')
    .then(response => {
      if (response.status !== 200) {
        console.log('Looks like there was a problem. Status Code: ' + response.status);
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
  const element = event.target;
  const currentFieldSet = element.closest('fieldset');
	const nextFieldSet = currentFieldSet.nextElementSibling;
  const progressbar = document.querySelector('#progressbar');
  const fieldsets = Array.prototype.slice.call(document.querySelectorAll('fieldset'));
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
  const fieldsets = Array.prototype.slice.call(document.querySelectorAll('fieldset'));
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
    console.log(input);
    input.onclick = previous;
  });

  setupInputValidations();
  refreshWifi();
  rtcUpdate();
};

/* main */
window.onload = ready;