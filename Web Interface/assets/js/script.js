/*  clock */
const hours = document.querySelector('.hours');
const minutes = document.querySelector('.minutes');
const seconds = document.querySelector('.seconds');

clock = () => {
    let today = new Date();
    let h = today.getHours() % 12 + today.getMinutes() / 59; // 22 % 12 = 10pm
    let m = today.getMinutes(); // 0 - 59
    let s = today.getSeconds(); // 0 - 59

    h *= 30; // 12 * 30 = 360deg
    m *= 6;
    s *= 6; // 60 * 6 = 360deg

    rotation(hours, h);
    rotation(minutes, m);
    rotation(seconds, s);

    // call every second
    setTimeout(clock, 500);
};

rotation = (target, val) => {
    target.style.transform = `rotate(${val}deg)`;
};

window.onload = clock();

function toggleDiv() {
    $('.components').toggle();
    $('.components2').toggle();
}

const firebaseConfig = {
    apiKey: "AIzaSyBj5nWN6kpJxqyIvAdpltV4fD9mb0GBUaQ",
    authDomain: "fish-feeder-01.firebaseapp.com",
    databaseURL: "https://fish-feeder-01-default-rtdb.firebaseio.com",
    projectId: "fish-feeder-01",
    storageBucket: "fish-feeder-01.appspot.com",
    messagingSenderId: "303334623682",
    appId: "1:303334623682:web:ccfbc72d08b8513bd6ee5b",
    measurementId: "G-7DMPL4K3W6"
};
firebase.initializeApp(firebaseConfig);


var temp = firebase.database().ref('dev1/temp');
temp.on('value', function(snapshot) {
    temp = snapshot.val()
    document.getElementById("Temp").innerText = temp;
});

var ph = firebase.database().ref('dev1/ph');
ph.on('value', function(snapshot) {
    ph = snapshot.val()
    document.getElementById("ph").innerText = ph>14? "pH Sensor Error":ph;
    console.log
});

var timestamp = firebase.database().ref('dev1/timestamp');
timestamp.on('value', function(snapshot) {
    timestamp = new Date(snapshot.val() * 1000).toISOString().slice(0, 19).replace('T', ' ')
     testTS = `<p >Last Updated: ${timestamp}</p> `
     document.getElementById("timeS").innerHTML = testTS;
    //  console.log(testTS);
     document.getElementById("timeS1").innerHTML = testTS;
});

var countRef = firebase.database().ref('dev1/count');
countRef.on('value', function(snapshot) {
    count = snapshot.val()
    // console.log(count)
});

function feednow() {
    firebase.database().ref('dev1').update({
        feednow: 1,
    });
}

$(document).ready(function() {
    $('#timepicker').mdtimepicker(); //Initializes the time picker
    addDiv();
});

$('#timepicker').mdtimepicker().on('timechanged', function(e) {
    console.log(e.time)
    addStore(count, e);
    count > 0 ? count = 1:count = count + 1
    firebase.database().ref('dev1/').update({
        count: parseInt(count),
    });
});

function addStore(count, e) {
    if(count >= 1){
        alert("Hold one! Only one schedule allowed!")
    } 
    else{
        firebase.database().ref('dev1/timers/timer' + count).set({
            time: e.time
        });
        addDiv();
    }
}

function showShort(id) {
    var idv = $(id)[0]['id']
    $("#time_" + idv).toggle();
    $("#short_" + idv).toggle();
}

function removeDiv(id) {
    var idv = $(id)[0]['id']
    firebase.database().ref('dev1/timers/' + idv).remove();
    if (count >= 0) {
        count = count - 1;
    }

    firebase.database().ref().update({
        count: parseInt(count),
    });
    $(id).fadeOut(1, 0).fadeTo(500, 0)
}

function addDiv() {
    var divRef = firebase.database().ref('dev1/timers');
    divRef.on('value', function(snapshot) {
        var obj = snapshot.val()
        var i = 0;
        $('#wrapper').html('')
        while (i <= count) {
            var propertyValues = Object.entries(obj);
            let ts = propertyValues[i][1]['time']
                //var timeString = "12:04";
            var H = +ts.substr(0, 2);
            var h = (H % 12) || 12;
            h = (h < 10) ? ("0" + h) : h; // leading 0 at the left for 1 digit hours
            var ampm = H < 12 ? " AM" : " PM";
            ts = h + ts.substr(2, 3) + ampm;
            // console.log(ts)

            const x = `
            <div id=${propertyValues[i][0]}>
                <div class="btn2 btn__secondary2" onclick=showShort(${propertyValues[i][0]}) id="main_${propertyValues[i][0]}">
                <div id="time_${propertyValues[i][0]}">
                ${ts}
                </div>
                <div class="icon2" id="short_${propertyValues[i][0]}" onclick=removeDiv(${propertyValues[i][0]})>
                    <div class="icon__add">
                        <ion-icon name="trash"></ion-icon>
                    </div>
                </div>
                </div>
                
                
            </div>`

            $('#wrapper').append(x);
            i++;
        }
    });
}