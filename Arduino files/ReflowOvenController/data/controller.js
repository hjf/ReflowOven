var tempchart;
var lastState;

async function startController() {
    console.log("Waiting for JQuery...");
    while (typeof $ === 'undefined') {
        await sleep(1);
    }
    console.log("Waiting for chart library...");
    while (typeof Chart === 'undefined') {
        await sleep(1);
    }

    console.log("All loaded")

    var ctx = document.getElementById("tempchart").getContext('2d');

    tempchart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: 'Temperatura',
                data: [],


                borderColor: 'rgb(255, 0, 0)',
                backgroundColor: 'rgb(255, 0, 0, 0.25)',
                fill: true

            }]
        },
        options: {
            elements: { point: { radius: 0 } },
            responsive: true,
            scales: {
                yAxes: [{
                    display: true,
                    scaleLabel: {
                        display: true,
                        labelString: 'ºC'
                    },
                    ticks: {
                        beginAtZero: true,
                        max: 300

                    },

                }],
                xAxes: [{
                    type: 'linear',
                    display: true,
                    scaleLabel: {
                        display: true,
                        labelString: 'Seconds'
                    },
                    ticks: {
                        min: 0,
                        suggestedMax: 50,
                        display: true
                    }
                }],
            }
        }
    })

    setInterval(function () { updateStatus(); }, 1000);

}
var i = 0;
function updateStatus() {

    $.ajax({
        url: "/api/status.json",
        context: document.body
    }).done(function (data) {

        var tct = Math.round(data.tct);
        $("#currentOvenTemperature").html(tct);
        $("#currentControllerTemperature").html(Math.round(data.tccj));

        $("#currentStatus").html(data.state);
        document.getElementById("powerLevel").value = Math.round(data.level * 100);

        if (!pidIsEditing) {

            document.getElementById("Kp").value = data.Kp;
            document.getElementById("Ki").value = data.Ki;
            document.getElementById("Kd").value = data.Kd;
        }
        lastState = data.state;
        //  $("#startButton").prop("disabled", "enabled");
        if (data.state != "IDLE") {
            tempchart.data.labels.push(i);
            tempchart.data.datasets[0].data.push({ x: i, y: tct });
            tempchart.update();
            $("#targetTemperature").html(Math.round(data.target));
            $("#startButton").prop("class", "red");
            $("#startButton").prop("value", "  STOP  ");
            // $("#startButton").prop("onclick", "startStop()");
            $("#remainingParagraph").removeClass("hide");
            if (data.state == "REFLOW" || data.state == "SOAK") {
                
                if (data.remainingSeconds === -1) {
                    $("#remainingSeconds").html("Waiting to reach temperature...");
                } else {
                    $("#remainingSeconds").html(data.remainingSeconds + "s");
                }
            } else {
                $("#remainingParagraph").addClass("hide");
            }
        } else {
            $("#startButton").prop("class", "green");
            $("#startButton").prop("value", "  START  ");
            //   $("#startButton").prop("onclick", "startStop()");
            $("#targetTemperature").html("---");

            $("#remainingParagraph").addClass("hide");
        }
        i++
    })

}

var pidIsEditing = false;
function toggleEditPID() {
    pidIsEditing = !pidIsEditing;
    $("#Kp").prop("disabled", !pidIsEditing);
    $("#Ki").prop("disabled", !pidIsEditing);
    $("#Kd").prop("disabled", !pidIsEditing);
    $("#submitPid").prop("disabled", !pidIsEditing);
}

function setPid() {
    var tosend = {
        'kp': $("#Kp").prop("value"),
        'ki': $("#Ki").prop("value"),
        'kd': $("#Kd").prop("value")
    }
    toggleEditPID();
    $.post({
        url: "/api/setPid",
        data: tosend,
        success: function (data) {

        }

    });
}

function startStop() {
    if (typeof lastState === 'undefined')
        return;
    var tosend = {
        action: "STOP"
    }

    if (lastState == "IDLE")
        tosend.action = "START";

    $.post({
        url: "/api/setMode",
        data: tosend,
        success: function (data) {

        }

    });
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}