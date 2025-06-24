document.addEventListener('DOMContentLoaded', function() {
    const updateInterval = 2000;
    let currentMode = 0;
    
    // Elements
    const tempCubeEl = document.getElementById('tempCube');
    const tempColumnEl = document.getElementById('tempColumn');
    const tempTsaEl = document.getElementById('tempTsa');
    const tempCoolantEl = document.getElementById('tempCoolant');
    const pressureEl = document.getElementById('pressure');
    const flowEl = document.getElementById('flow');
    const strengthEl = document.getElementById('strength');
    const leakEl = document.getElementById('leak');
    const currentAngleEl = document.getElementById('currentAngle');
    const angleSlider = document.getElementById('angleSlider');
    const applyAngleBtn = document.getElementById('applyAngle');
    
    // Mode buttons
    document.querySelectorAll('.btn[data-mode]').forEach(btn => {
        btn.addEventListener('click', function() {
            const mode = this.getAttribute('data-mode');
            setMode(mode);
        });
    });
    
    // Emergency stop
    document.querySelector('.btn.emergency').addEventListener('click', function() {
        setMode('8'); // MODE_EMERGENCY_STOP
    });
    
    // Calibration buttons
    document.querySelectorAll('.btn.calibrate').forEach(btn => {
        btn.addEventListener('click', function() {
            const type = this.getAttribute('data-type');
            calibrate(type);
        });
    });
    
    // Manual angle control
    angleSlider.addEventListener('input', function() {
        currentAngleEl.textContent = this.value;
    });
    
    applyAngleBtn.addEventListener('click', function() {
        setServoAngle(angleSlider.value);
    });
    
    // Update sensor data
    function updateSensorData() {
        fetch('/sensors')
            .then(response => response.json())
            .then(data => {
                tempCubeEl.textContent = data.cube.toFixed(2);
                tempColumnEl.textContent = data.column.toFixed(2);
                tempTsaEl.textContent = data.tsa.toFixed(2);
                tempCoolantEl.textContent = data.coolant.toFixed(2);
                pressureEl.textContent = data.pressure.toFixed(0);
                flowEl.textContent = data.flow.toFixed(2);
                strengthEl.textContent = data.strength.toFixed(1);
                leakEl.textContent = data.leak ? "Yes" : "No";
                currentMode = data.mode;
                
                // Update UI based on mode
                document.querySelectorAll('.btn[data-mode]').forEach(btn => {
                    btn.classList.remove('active');
                    if(parseInt(btn.getAttribute('data-mode')) === data.mode) {
                        btn.classList.add('active');
                    }
                });
            })
            .catch(error => console.error('Error:', error));
    }
    
    // Set system mode
    function setMode(mode) {
        fetch('/setmode', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
            },
            body: `mode=${mode}`
        });
    }
    
    // Calibrate servo
    function calibrate(type) {
        fetch(`/calibrate/${type}`, { method: 'POST' })
            .then(response => response.text())
            .then(data => alert(data));
    }
    
    // Set servo angle
    function setServoAngle(angle) {
        fetch('/setangle?angle=' + angle);
    }
    
    // Initial update and set interval
    updateSensorData();
    setInterval(updateSensorData, updateInterval);
});
// Калькулятор продукта
document.getElementById('calculateBtn').addEventListener('click', function() {
  const volume = parseFloat(document.getElementById('volume').value);
  const abv = parseFloat(document.getElementById('abv').value);
  
  if (isNaN(volume) || isNaN(abv)) {
    alert('Please enter valid numbers');
    return;
  }
  
  // Рассчеты
  const totalAlcohol = volume * (abv / 100);
  const pureAlcohol = totalAlcohol * 0.95; // Эффективность 95%
  
  // Распределение фракций (можно настроить коэффициенты)
  const headsPercentage = 0.1; // 10%
  const tailsPercentage = 0.2; // 20%
  const bodyPercentage = 0.7; // 70%
  
  const headsVolume = pureAlcohol * headsPercentage;
  const bodyVolume = pureAlcohol * bodyPercentage;
  const tailsVolume = pureAlcohol * tailsPercentage;
  
  // Обновление UI
  document.getElementById('totalAlcohol').textContent = totalAlcohol.toFixed(2);
  document.getElementById('pureAlcohol').textContent = pureAlcohol.toFixed(2);
  
  document.getElementById('headsVolume').textContent = headsVolume.toFixed(2);
  document.getElementById('headsPercent').textContent = (headsPercentage * 100).toFixed(0);
  
  document.getElementById('bodyVolume').textContent = bodyVolume.toFixed(2);
  document.getElementById('bodyPercent').textContent = (bodyPercentage * 100).toFixed(0);
  
  document.getElementById('tailsVolume').textContent = tailsVolume.toFixed(2);
  document.getElementById('tailsPercent').textContent = (tailsPercentage * 100).toFixed(0);
});